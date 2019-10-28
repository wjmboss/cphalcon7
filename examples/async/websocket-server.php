<?php

class Websocket
{
	static public $debug = false;
	protected $port = null;
	protected $host = null;
	protected $server = null;
	protected $callback = null;

	protected static $opcodes = array(
		'continuation' => 0,
		'text'         => 1,
		'binary'       => 2,
		'close'        => 8,
		'ping'         => 9,
		'pong'         => 10,
	);

	public function __construct($host, $port, $callback = NULL) {
		$this->port = $port;
		$this->host = $host;
		$this->callback = $callback;
	}

	public function start()
	{
		try {
			$this->server = \Phalcon\Async\Network\TcpServer::listen($this->host, $this->port);
			echo Phalcon\Cli\Color::info('start server listen:'.$this->host.':'.$this->port).PHP_EOL;
			while (true) {
				$socket = $this->server->accept();
				if ($socket === false) {
					continue;
				}
				$callback = $this->callback;
				\Phalcon\Async\Task::async(function () use ($socket, $callback) {
					$isClose = false;
					$isHandshake = false;
					$socket->is_closing = false;
					$socket->fragment_status = 0;
					$socket->fragment_length = 0;
					$socket->fragment_size = 4096;
					$socket->read_length = 0;
					$socket->huge_payload = '';
					$socket->payload = '';
					$socket->headers = NULL;
					$socket->request_path = NULL;
					try {
						$buffer = '';
						while (!$socket->is_closing && null !== ($chunk = $socket->read())) {

							$buffer .= $chunk;
							if ($isHandshake === false) {
								$pos = strpos($buffer, "\r\n\r\n");
								if ($pos) {
									if ($this->handShake($socket, $buffer)) {
										$isHandshake = true;
										$buffer = substr($buffer, $pos+4);
									}
								}
								continue;
							}
							if ($this->process($socket, $buffer)) {
								$buffer = substr($buffer, $socket->read_length);
								if ($callback && \is_callable($callback)) {
									$callback($socket, $socket->headers, $socket->request_path, $socket->payload);
								}
								$socket->fragment_status = 0;
								$socket->fragment_length = 0;
								$socket->read_length = 0;
								$socket->huge_payload = '';
								$socket->payload = '';
							}
						}
					} catch (\Throwable $e) {
						self::err($e->getMessage());
					} finally {
						$socket->close();
					}
				});
			}
		} catch (\Throwable $e) {
			self::err($e->getMessage());
		} finally {
			if ($this->server) {
				$this->server->close();
			}
		}
	}

	/**
	 * 请求握手
	 * @return boolean
	 */
	static public function handShake($socket, $buffer)
	{
		self::info('recv:'.$buffer);
		if (($wsKeyIndex = stripos($buffer, 'Sec-WebSocket-Key:')) === false) {
			throw new \Exception('Handshake failed, Sec-WebSocket-Key missing');
		}

		if (!preg_match('/GET (.*) HTTP\//mUi', $buffer, $matches)) {
			throw new \Exception('Handshake failed, No GET in HEAD');
		}
		$uri = trim($matches[1]);
		$uri_parts = parse_url($uri);;

		$socket->headers = explode("\r\n", $buffer);
		$socket->request_path = $uri_parts['path'];

		$wsKey = substr($buffer, $wsKeyIndex + 18);
		$key = trim(substr($wsKey, 0, strpos($wsKey, "\r\n")));

		// 根据客户端传递过来的 key 生成 accept key
		$acceptKey = base64_encode(sha1($key . "258EAFA5-E914-47DA-95CA-C5AB0DC85B11", true));

		// 拼接回复字符串
		$msg = "HTTP/1.1 101 Switching Protocols\r\n";
		$msg .= "Upgrade: websocket\r\n";
		$msg .= "Sec-WebSocket-Version: 13\r\n";
		$msg .= "Connection: Upgrade\r\n";
		$msg .= "Sec-WebSocket-Accept: " . $acceptKey . "\r\n\r\n";
		$socket->write($msg);
		return true;
	}

	static public function readFragment0($socket, $buffer) {
		if (strlen($buffer) < 2) {
			return false;
		}
		$data = substr($buffer, 0, 2);
		$socket->read_length = 2;
		// Is this the final fragment?	// Bit 0 in byte 0
		/// @todo Handle huge payloads with multiple fragments.
		$socket->final = (boolean) (ord($data[0]) & 1 << 7);

		// Should be unused, and must be false…	// Bits 1, 2, & 3
		$rsv1	= (boolean) (ord($data[0]) & 1 << 6);
		$rsv2	= (boolean) (ord($data[0]) & 1 << 5);
		$rsv3	= (boolean) (ord($data[0]) & 1 << 4);

		// Parse opcode
		$opcode_int = ord($data[0]) & 31; // Bits 4-7
		$opcode_ints = array_flip(self::$opcodes);
		if (!array_key_exists($opcode_int, $opcode_ints)) {
			throw new \Exception('Bad opcode in websocket frame: '.$opcode_int);
		}
		$opcode = $opcode_ints[$opcode_int];

		// record the opcode if we are not receiving a continutation fragment
		if ($opcode !== 'continuation') {
			$socket->last_opcode = $opcode;
		}

		// Masking?
		$socket->mask = (boolean) (ord($data[1]) >> 7);	// Bit 0 in byte 1

		$socket->payload = '';

		// Payload length
		$socket->payload_length = (integer) ord($data[1]) & 127; // Bits 1-7 in byte 1
		if ($socket->payload_length > 125) {
			$socket->fragment_status = 1;
		} else {
			$socket->fragment_status = 2;
		}
		return true;
	}

	static public function readFragment1($socket, $buffer) {
		if ($socket->payload_length === 126) {
			if ($socket->fragment_length - $socket->read_length < 2) {
				return false;
			}
			$data = substr($buffer, $socket->read_length, 2); // 126: Payload is a 16-bit unsigned int
			$socket->read_length += 2;
		} else {
			if ($socket->fragment_length - $socket->read_length < 8) {
				return false;
			}
			$data = substr($buffer, $socket->read_length, 8); // 127: Payload is a 64-bit unsigned int
			$socket->read_length += 8;
		}
		$socket->payload_length = bindec(self::sprintB($data));
		$socket->fragment_status = 2;
		return true;
	}

	static public function readFragment2($socket, $buffer) {

		// Get masking key.
		if ($socket->mask) {
			if ($socket->fragment_length - $socket->read_length < (4 + $socket->payload_length)) {
				return false;
			}
			$masking_key = substr($buffer, $socket->read_length, 4);
			$socket->read_length += 4;
		} elseif ($socket->fragment_length - $socket->read_length < $socket->payload_length) {
			return false;
		}

		// Get the actual payload, if any (might not be for e.g. close frames.
		if ($socket->payload_length > 0) {
			$data = substr($buffer, $socket->read_length, $socket->payload_length);
			$socket->read_length += $socket->payload_length;

			if ($socket->mask) {
				// Unmask payload.
				for ($i = 0; $i < $socket->payload_length; $i++) {
					$socket->payload .= ($data[$i] ^ $masking_key[$i % 4]);
				}
			} else {
				$socket->payload = $data;
			}
		}
		$socket->fragment_status = 3;
		return true;
	}

	static public function sendFragment($socket, $payload, $opcode = 'text', $masked = true) {
		if (!in_array($opcode, array_keys(self::$opcodes))) {
			throw new \Exception('Bad opcode '.$opcode.', try text or binary.');
		}

		// record the length of the payload
		$payload_length = strlen($payload);

		$fragment_cursor = 0;
		// while we have data to send
		while ($payload_length > $fragment_cursor) {
			// get a fragment of the payload
			$sub_payload = substr($payload, $fragment_cursor, $socket->fragment_size);

			// advance the cursor
			$fragment_cursor += $socket->fragment_size;

			// is this the final fragment to send?
			$final = $payload_length <= $fragment_cursor;

			// send the fragment
			self::send_frame($socket, $final, $sub_payload, $opcode, $masked);

			// all fragments after the first will be marked a continuation
			$opcode = 'continuation';
		}

	}

	static public function send_frame($socket, $final, $payload, $opcode, $masked) {
		// Binary string for header.
		$frame_head_binstr = '';

		// Write FIN, final fragment bit.
		$frame_head_binstr .= (bool) $final ? '1' : '0';

		// RSV 1, 2, & 3 false and unused.
		$frame_head_binstr .= '000';

		// Opcode rest of the byte.
		$frame_head_binstr .= sprintf('%04b', self::$opcodes[$opcode]);

		// Use masking?
		$frame_head_binstr .= $masked ? '1' : '0';

		// 7 bits of payload length...
		$payload_length = strlen($payload);
		if ($payload_length > 65535) {
			$frame_head_binstr .= decbin(127);
			$frame_head_binstr .= sprintf('%064b', $payload_length);
		}
		elseif ($payload_length > 125) {
			$frame_head_binstr .= decbin(126);
			$frame_head_binstr .= sprintf('%016b', $payload_length);
		}
		else {
			$frame_head_binstr .= sprintf('%07b', $payload_length);
		}

		$frame = '';

		// Write frame head to frame.
		foreach (str_split($frame_head_binstr, 8) as $binstr) $frame .= chr(bindec($binstr));

		// Handle masking
		if ($masked) {
			// generate a random mask:
			$mask = '';
			for ($i = 0; $i < 4; $i++) $mask .= chr(rand(0, 255));
			$frame .= $mask;
		}

		// Append payload to frame:
		for ($i = 0; $i < $payload_length; $i++) {
			$frame .= ($masked === true) ? $payload[$i] ^ $mask[$i % 4] : $payload[$i];
		}

		$socket->write($frame);
	}

	/**
	 * Helper to convert a binary to a string of '0' and '1'.
	 */
	protected static function sprintB($string) {
		$ret = '';
		for ($i = 0; $i < strlen($string); $i++) {
			$ret .= sprintf("%08b", ord($string[$i]));
		}
		return $ret;
	}

	/**
	 * 解析数据包
	 *
	 * @return string
	 */
	static public function process($socket, $buffer)
	{
startfragment:
		self::info('process fragment_status:'.$socket->fragment_status.' buffer length:'.strlen($buffer));
		$socket->fragment_length = strlen($buffer);

		switch ($socket->fragment_status) {
			case 0:
				// Just read the main fragment information first.
				if (!self::readFragment0($socket, $buffer)) {
					return false;
				}
				goto startfragment;
				break;
			case 1:
				if (!self::readFragment1($socket, $buffer)) {
					return false;
				}
				goto startfragment;
				break;
			case 2:
				if (!self::readFragment2($socket, $buffer)) {
					return false;
				}
				goto startfragment;
				break;
			case 3:
			{
				if ($socket->last_opcode === 'close') {
					// Get the close status.
					if ($socket->payload_length >= 2) {
						$status_bin = $socket->payload[0] . $socket->payload[1];
						$status = bindec(sprintf("%08b%08b", ord($socket->payload[0]), ord($socket->payload[1])));
						$socket->close_status = $status;
						$socket->payload = substr($socket->payload, 2);

						self::sendFragment($socket, $status_bin . 'Close acknowledged: ' . $status, 'close', true); // Respond.
					}

					$socket->is_closing = true; // A close response, all done.
				}

				// if this is not the last fragment, then we need to save the payload
				if (!$socket->final) {
					$socket->huge_payload .= $socket->payload;
					self::info('final:'.$socket->final.', payload:'.$socket->payload);
					return false;
				} else {
					// sp we need to retreive the whole payload
					$socket->huge_payload .= $socket->payload;
					$socket->payload = $socket->huge_payload;
					$socket->huge_payload = null;
					self::info('final:'.$socket->final.', payload:'.$socket->payload);
					return true;
				}
				break;
			}
		}
		return false;
	}

	static public function info($message)
	{
		if (self::$debug) {
			echo Phalcon\Cli\Color::info($message).PHP_EOL;
		}
	}

	static public function err($message)
	{
		echo Phalcon\Cli\Color::error($message).PHP_EOL;
	}
}

/**
 * 客户端测试
 * sudo apt install node-ws
 * wscat -c ws://localhost:10001
 */
// Websocket::$debug = true;
$ws = new Websocket('0.0.0.0', 10001, function($socket, $headers, $path, $data) {
	var_dump($data);
	Websocket::sendFragment($socket, 'Re: '.$data);
});
$ws->start();