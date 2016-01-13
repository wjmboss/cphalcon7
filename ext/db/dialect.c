
/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2014 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  +------------------------------------------------------------------------+
*/

#include "db/dialect.h"
#include "db/dialectinterface.h"
#include "db/exception.h"

#include "kernel/main.h"
#include "kernel/memory.h"
#include "kernel/operators.h"
#include "kernel/fcall.h"
#include "kernel/concat.h"
#include "kernel/object.h"
#include "kernel/array.h"
#include "kernel/string.h"
#include "kernel/exception.h"
#include "kernel/hash.h"

#include "interned-strings.h"

/**
 * Phalcon\Db\Dialect
 *
 * This is the base class to each database dialect. This implements
 * common methods to transform intermediate code into its RDBM related syntax
 */
zend_class_entry *phalcon_db_dialect_ce;

PHP_METHOD(Phalcon_Db_Dialect, limit);
PHP_METHOD(Phalcon_Db_Dialect, forUpdate);
PHP_METHOD(Phalcon_Db_Dialect, sharedLock);
PHP_METHOD(Phalcon_Db_Dialect, getColumnList);
PHP_METHOD(Phalcon_Db_Dialect, getSqlExpression);
PHP_METHOD(Phalcon_Db_Dialect, getSqlExpressionCase);
PHP_METHOD(Phalcon_Db_Dialect, getSqlExpressionFunctionCall);
PHP_METHOD(Phalcon_Db_Dialect, getSqlTable);
PHP_METHOD(Phalcon_Db_Dialect, select);
PHP_METHOD(Phalcon_Db_Dialect, insert);
PHP_METHOD(Phalcon_Db_Dialect, update);
PHP_METHOD(Phalcon_Db_Dialect, delete);
PHP_METHOD(Phalcon_Db_Dialect, supportsSavepoints);
PHP_METHOD(Phalcon_Db_Dialect, supportsReleaseSavepoints);
PHP_METHOD(Phalcon_Db_Dialect, createSavepoint);
PHP_METHOD(Phalcon_Db_Dialect, releaseSavepoint);
PHP_METHOD(Phalcon_Db_Dialect, rollbackSavepoint);
PHP_METHOD(Phalcon_Db_Dialect, getEscapeChar);;
PHP_METHOD(Phalcon_Db_Dialect, registerCustomFunction);
PHP_METHOD(Phalcon_Db_Dialect, getCustomFunctions);

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_db_dialect_getsqlexpression, 0, 0, 1)
	ZEND_ARG_INFO(0, expression)
	ZEND_ARG_INFO(0, escapeChar)
	ZEND_ARG_INFO(0, quoting)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_db_dialect_getsqlexpressioncase, 0, 0, 1)
	ZEND_ARG_INFO(0, expression)
	ZEND_ARG_INFO(0, escapeChar)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_db_dialect_getsqlexpressionfunctioncall, 0, 0, 1)
	ZEND_ARG_INFO(0, expression)
	ZEND_ARG_INFO(0, escapeChar)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_db_dialect_getsqltable, 0, 0, 1)
	ZEND_ARG_INFO(0, table)
	ZEND_ARG_INFO(0, escapeChar)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_phalcon_db_dialect_registercustomfunction, 0, 0, 2)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, customFunction)
ZEND_END_ARG_INFO()

static const zend_function_entry phalcon_db_dialect_method_entry[] = {
	PHP_ME(Phalcon_Db_Dialect, limit, arginfo_phalcon_db_dialectinterface_limit, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, forUpdate, arginfo_phalcon_db_dialectinterface_forupdate, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, sharedLock, arginfo_phalcon_db_dialectinterface_sharedlock, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, getColumnList, arginfo_phalcon_db_dialectinterface_getcolumnlist, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, getSqlExpression, arginfo_phalcon_db_dialect_getsqlexpression, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, getSqlExpressionCase, arginfo_phalcon_db_dialect_getsqlexpressioncase, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, getSqlExpressionFunctionCall, arginfo_phalcon_db_dialect_getsqlexpressionfunctioncall, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, getSqlTable, arginfo_phalcon_db_dialect_getsqltable, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, select, arginfo_phalcon_db_dialectinterface_select, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, insert, arginfo_phalcon_db_dialectinterface_insert, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, update, arginfo_phalcon_db_dialectinterface_update, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, delete, arginfo_phalcon_db_dialectinterface_delete, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, supportsSavepoints, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, supportsReleaseSavepoints, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, createSavepoint, arginfo_phalcon_db_dialectinterface_createsavepoint, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, releaseSavepoint, arginfo_phalcon_db_dialectinterface_releasesavepoint, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, rollbackSavepoint, arginfo_phalcon_db_dialectinterface_rollbacksavepoint, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, getEscapeChar, NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, registerCustomFunction, arginfo_phalcon_db_dialect_registercustomfunction, ZEND_ACC_PUBLIC)
	PHP_ME(Phalcon_Db_Dialect, getCustomFunctions, NULL, ZEND_ACC_PUBLIC)
	PHP_FE_END
};

/**
 * Phalcon\Db\Dialect initializer
 */
PHALCON_INIT_CLASS(Phalcon_Db_Dialect){

	PHALCON_REGISTER_CLASS(Phalcon\\Db, Dialect, db_dialect, phalcon_db_dialect_method_entry, ZEND_ACC_EXPLICIT_ABSTRACT_CLASS);

	zend_declare_property_null(phalcon_db_dialect_ce, SL("_escapeChar"), ZEND_ACC_PROTECTED);
	zend_declare_property_null(phalcon_db_dialect_ce, SL("_customFunctions"), ZEND_ACC_PROTECTED);

	zend_class_implements(phalcon_db_dialect_ce, 1, phalcon_db_dialectinterface_ce);

	return SUCCESS;
}

/**
 * Generates the SQL for LIMIT clause
 *
 *<code>
 * $sql = $dialect->limit('SELECT * FROM robots', 10);
 * echo $sql; // SELECT * FROM robots LIMIT 10
 *</code>
 *
 * @param string $sqlQuery
 * @param int $number
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, limit){

	zval *sql_query, *number, *limit;

	phalcon_fetch_params(1, 2, 0, &sql_query, &number);

	if (phalcon_is_numeric(number)) {
		PHALCON_MM_GROW();
		PHALCON_INIT_VAR(limit);
		ZVAL_LONG(limit, phalcon_get_intval(number));
		PHALCON_CONCAT_VSV(return_value, sql_query, " LIMIT ", limit);
		RETURN_MM();
	}

	RETURN_ZVAL(sql_query, 1, 0);
}

/**
 * Returns a SQL modified with a FOR UPDATE clause
 *
 *<code>
 * $sql = $dialect->forUpdate('SELECT * FROM robots');
 * echo $sql; // SELECT * FROM robots FOR UPDATE
 *</code>
 *
 * @param string $sqlQuery
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, forUpdate){

	zval *sql_query;

	phalcon_fetch_params(0, 1, 0, &sql_query);

	PHALCON_CONCAT_VS(return_value, sql_query, " FOR UPDATE");
	return;
}

/**
 * Returns a SQL modified with a LOCK IN SHARE MODE clause
 *
 *<code>
 * $sql = $dialect->sharedLock('SELECT * FROM robots');
 * echo $sql; // SELECT * FROM robots LOCK IN SHARE MODE
 *</code>
 *
 * @param string $sqlQuery
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, sharedLock){

	zval *sql_query;

	phalcon_fetch_params(0, 1, 0, &sql_query);

	PHALCON_CONCAT_VS(return_value, sql_query, " LOCK IN SHARE MODE");
	return;
}

/**
 * Gets a list of columns with escaped identifiers
 *
 *<code>
 * echo $dialect->getColumnList(array('column1', 'column'));
 *</code>
 *
 * @param array $columnList
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, getColumnList){

	zval *column_list, *str_list, *escape_char, *column = NULL;
	zval *column_quoted = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &column_list);

	PHALCON_INIT_VAR(str_list);
	array_init(str_list);

	escape_char = phalcon_read_property(getThis(), SL("_escapeChar"), PH_NOISY);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(column_list), column) {
		PHALCON_INIT_NVAR(column_quoted);
		PHALCON_CONCAT_VVV(column_quoted, escape_char, column, escape_char);
		phalcon_array_append(str_list, column_quoted, PH_COPY);
	} ZEND_HASH_FOREACH_END();

	phalcon_fast_join_str(return_value, SL(", "), str_list);
	RETURN_MM();
}

/**
 * Return the escape char
 *
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, getEscapeChar){


	RETURN_MEMBER(getThis(), "_escapeChar");
}

/**
 * Transforms an intermediate representation for a expression into a database system valid expression
 *
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, getSqlExpression){

	zval *expression, *escape_char = NULL, *quoting = NULL, type, name, escaped_name;
	zval domain, value, *expression_value = NULL, *operator = NULL;
	zval times, placeholders, left, right, *expression_left = NULL, *expression_right = NULL;
	zval sql_items, *items, *item, *item_expression = NULL;
	zval list_expression, *exception_message;
	int t, i;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 2, &expression, &escape_char, &quoting);

	if (Z_TYPE_P(expression) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid SQL expression");
		return;
	}

	if (!escape_char) {
		PHALCON_INIT_VAR(escape_char);
	} else {
		PHALCON_SEPARATE_PARAM(escape_char);
	}

	if (!quoting) {
		quoting = &PHALCON_GLOBAL(z_false);
	}

	if (PHALCON_GLOBAL(db).escape_identifiers) {
		if (Z_TYPE_P(escape_char) == IS_NULL) {
			escape_char = phalcon_read_property(getThis(), SL("_escapeChar"), PH_NOISY);
		}
	}

	if (!phalcon_array_isset_str(expression, SL("type"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid SQL expression");
		return;
	}

	phalcon_array_fetch_str(&type, expression, SL("type"), PH_NOISY);

	/**
	 * Resolve qualified expressions
	 */
	if (PHALCON_IS_STRING(&type, "qualified")) {
		phalcon_array_fetch_str(&name, expression, SL("name"), PH_NOISY);
		if (PHALCON_GLOBAL(db).escape_identifiers) {
			PHALCON_CONCAT_VVV(&escaped_name, escape_char, &name, escape_char);
		} else {
			ZVAL_COPY_VALUE(&escaped_name, &name);
		}

		/**
		 * A domain could be a table/schema
		 */
		if (phalcon_array_isset_fetch_str(&domain, expression, SL("domain"))) {
			if (PHALCON_GLOBAL(db).escape_identifiers) {
				PHALCON_CONCAT_VVVSV(return_value, escape_char, &domain, escape_char, ".", &escaped_name);
			} else {
				PHALCON_CONCAT_VSV(return_value, &domain, ".", &escaped_name);
			}

			RETURN_MM();
		}

		RETURN_CTOR(&escaped_name);
	}

	/**
	 * Resolve literal expressions
	 */
	if (PHALCON_IS_STRING(type, "literal")) {
		phalcon_array_fetch_str(&value, expression, SL("value"), PH_NOISY);

		if (zend_is_true(quoting)) {
			PHALCON_CONCAT_SVS(return_value, "'", &value, "'");
			RETURN_MM();
		}

		RETURN_CTOR(&value);
	}

	/**
	 * Resolve binary operations expressions
	 */
	if (PHALCON_IS_STRING(type, "binary-op")) {
		phalcon_array_fetch_str(&operator, expression, SL("op"), PH_NOISY);
		phalcon_array_fetch_str(&left, expression, SL("left"), PH_NOISY);
		phalcon_array_fetch_str(&right, expression, SL("right"), PH_NOISY);

		PHALCON_CALL_METHOD(&expression_left, getThis(), "getsqlexpression", &left, escape_char);
		PHALCON_CALL_METHOD(&expression_right, getThis(), "getsqlexpression", &right, escape_char);

		PHALCON_CONCAT_VSVSV(return_value, expression_left, " ", &operator, " ", expression_right);
		RETURN_MM();
	}

	/**
	 * Resolve unary operations expressions
	 */
	if (PHALCON_IS_STRING(type, "unary-op")) {
		phalcon_array_fetch_str(&operator, expression, SL("op"), PH_NOISY);

		/**
		 * Some unary operators uses the left operand...
		 */
		if (phalcon_array_isset_fetch_str(&left, expression, SL("left"))) {
			PHALCON_CALL_METHOD(&expression_left, getThis(), "getsqlexpression", &left, escape_char);
			PHALCON_CONCAT_VV(return_value, expression_left, &operator);
			RETURN_MM();
		}

		/**
		 * ...Others uses the right operand
		 */
		if (phalcon_array_isset_fetch_str(&right, expression, SL("right"))) {
			PHALCON_CALL_METHOD(&expression_right, getThis(), "getsqlexpression", &right, escape_char);
			PHALCON_CONCAT_VV(return_value, &operator, expression_right);
			RETURN_MM();
		}
	}

	/**
	 * Resolve placeholder
	 */
	if (PHALCON_IS_STRING(type, "placeholder")) {
		phalcon_array_fetch_str(&value, expression, SL("value"), PH_NOISY);

		if (phalcon_array_isset_fetch_str(&times, expression, SL("times"))) {
			array_init(&placeholders);

			t = phalcon_get_intval(&times);
			i = 0;
			while (i++ < t) {
				zval index, placeholder;
				ZVAL_LONG(&index, t);

				PHALCON_CONCAT_VV(&placeholder, &value, &index);
				
				phalcon_array_append(placeholders, &placeholder, PH_COPY);
			}

			phalcon_fast_join_str(&value, SL(", "), placeholders);
		}
		
		RETURN_CTOR(&value);
	}

	/**
	 * Resolve parentheses
	 */
	if (PHALCON_IS_STRING(type, "parentheses")) {
		phalcon_array_fetch_str(&left, expression, SL("left"), PH_NOISY);

		PHALCON_CALL_METHOD(&expression_left, getThis(), "getsqlexpression", &left, escape_char);

		PHALCON_CONCAT_SVS(return_value, "(", expression_left, ")");
		RETURN_MM();
	}

	/**
	 * Resolve function calls
	 */
	if (PHALCON_IS_STRING(type, "functionCall")) {
		PHALCON_RETURN_CALL_METHOD(getThis(), "getsqlexpressionfunctioncall", expression, escape_char);
		RETURN_MM();
	}

	/**
	 * Resolve lists
	 */
	if (PHALCON_IS_STRING(type, "list")) {
		array_init(&sql_items);

		phalcon_array_fetch_long(&items, expression, 0, PH_NOISY);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(items), item) {
			PHALCON_CALL_METHOD(&item_expression, getThis(), "getsqlexpression", item, escape_char);
			phalcon_array_append(&sql_items, item_expression, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_fast_join_str(&list_expression, SL(", "), &sql_items);
		PHALCON_CONCAT_SVS(return_value, "(", &list_expression, ")");

		RETURN_MM();
	}

	/**
	 * Resolve *
	 */
	if (PHALCON_IS_STRING(type, "all")) {
		RETURN_MM_STRING("*");
	}

	/**
	 * Resolve CAST of values 
	 */
	if (PHALCON_IS_STRING(type, "cast")) {
		PHALCON_OBS_NVAR(left);
		phalcon_array_fetch_str(&left, expression, SL("left"), PH_NOISY);

		PHALCON_CALL_METHOD(&expression_left, getThis(), "getsqlexpression", left, escape_char);

		PHALCON_OBS_NVAR(right);
		phalcon_array_fetch_str(&right, expression, SL("right"), PH_NOISY);

		PHALCON_CALL_METHOD(&expression_right, getThis(), "getsqlexpression", right, escape_char);
		PHALCON_CONCAT_SVSVS(return_value, "CAST(", expression_left, " AS ", expression_right, ")");
		RETURN_MM();
	}

	/**
	 * Resolve CASE of values 
	 */
	if (PHALCON_IS_STRING(type, "case")) {
		PHALCON_RETURN_CALL_METHOD(getThis(), "getsqlexpressioncase", expression, escape_char);
		RETURN_MM();
	}

	/**
	 * Resolve CONVERT of values encodings
	 */
	if (PHALCON_IS_STRING(type, "convert")) {
		PHALCON_OBS_NVAR(left);
		phalcon_array_fetch_str(&left, expression, SL("left"), PH_NOISY);

		PHALCON_CALL_METHOD(&expression_left, getThis(), "getsqlexpression", left, escape_char);

		PHALCON_OBS_NVAR(right);
		phalcon_array_fetch_str(&right, expression, SL("right"), PH_NOISY);

		PHALCON_CALL_METHOD(&expression_right, getThis(), "getsqlexpression", right, escape_char);
		PHALCON_CONCAT_SVSVS(return_value, "CONVERT(", expression_left, " USING ", expression_right, ")");
		RETURN_MM();
	}

	/**
	 * Resolve SELECT
	 */
	if (PHALCON_IS_STRING(type, "select")) {
		phalcon_array_fetch_str(&value, expression, SL("value"), PH_NOISY);

		PHALCON_CALL_METHOD(&expression_value, getThis(), "select", &value);

		PHALCON_CONCAT_SVS(return_value, "(", expression_value, ")");
		RETURN_MM();
	}

	/**
	 * Expression type wasn't found
	 */
	PHALCON_INIT_VAR(exception_message);
	PHALCON_CONCAT_SVS(exception_message, "Invalid SQL expression type '", type, "'");
	PHALCON_THROW_EXCEPTION_ZVAL(phalcon_db_exception_ce, exception_message);
	return;
}

/**
 * Resolve CASE expressions
 *
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, getSqlExpressionCase){

	zval *expression, *escape_char = NULL, *expr = NULL, *clauses, *clause = NULL;
	zval *clause_expr = NULL, *clause_when = NULL, *clause_then = NULL, *type = NULL, *tmp = NULL, *tmp1 = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &expression, &escape_char);

	if (!escape_char) {
		PHALCON_INIT_VAR(escape_char);
	} else {
		PHALCON_SEPARATE_PARAM(escape_char);
	}

	ZVAL_STRING(return_value, "CASE ");

	PHALCON_OBS_NVAR(expr);
	phalcon_array_fetch_str(&expr, expression, SL("expr"), PH_NOISY);

	PHALCON_CALL_METHOD(&tmp, getThis(), "getsqlexpression", expr, escape_char);

	PHALCON_CONCAT_SV(return_value, "CASE ", tmp);

	PHALCON_OBS_VAR(clauses);
	phalcon_array_fetch_str(&clauses, expression, SL("when-clauses"), PH_NOISY);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(clauses), clause) {
		PHALCON_OBS_NVAR(type);
		phalcon_array_fetch_str(&type, clause, ISL(type), PH_NOISY);

		if (PHALCON_IS_STRING(type, "when")) {

			PHALCON_OBS_NVAR(clause_when);
			phalcon_array_fetch_str(&clause_when, clause, SL("when"), PH_NOISY);

			PHALCON_CALL_METHOD(&tmp, getThis(), "getsqlexpression", clause_when, escape_char);

			PHALCON_OBS_NVAR(clause_then);
			phalcon_array_fetch_str(&clause_then, clause, SL("then"), PH_NOISY);

			PHALCON_CALL_METHOD(&tmp1, getThis(), "getsqlexpression", clause_then, escape_char);

			PHALCON_SCONCAT_SVSV(return_value, " WHEN ", tmp, " THEN ", tmp1);
		} else {
			PHALCON_OBS_NVAR(clause_expr);
			phalcon_array_fetch_str(&clause_expr, clause, ISL(expr), PH_NOISY);

			PHALCON_CALL_METHOD(&tmp, getThis(), "getsqlexpression", clause_expr, escape_char);

			PHALCON_SCONCAT_SV(return_value, " ELSE ", tmp);
		}
	} ZEND_HASH_FOREACH_END();

	PHALCON_SCONCAT_STR(return_value, " END");

	RETURN_MM();
}

/**
 * Resolve function calls
 *
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, getSqlExpressionFunctionCall){

	zval *expression, *escape_char = NULL, *name, *custom_functions = NULL, custom_function, *sql_arguments, arguments;
	zval *argument = NULL, *argument_expression = NULL, *arguments_joined;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &expression, &escape_char);

	if (!escape_char) {
		PHALCON_INIT_VAR(escape_char);
	} else {
		PHALCON_SEPARATE_PARAM(escape_char);
	}

	PHALCON_OBS_VAR(name);
	phalcon_array_fetch_str(&name, expression, SL("name"), PH_NOISY);

	PHALCON_CALL_METHOD(&custom_functions, getThis(), "getcustomfunctions");

	if (phalcon_array_isset_fetch(&custom_function, custom_functions, name)) {
		PHALCON_RETURN_CALL_ZVAL_FUNCTION(&custom_function, getThis(), expression, escape_char);
		RETURN_MM();
	}

	PHALCON_INIT_VAR(sql_arguments);
	array_init(sql_arguments);
	if (phalcon_array_isset_str(expression, SL("arguments"))) {
		phalcon_array_fetch_str(&arguments, expression, SL("arguments"), PH_NOISY);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&arguments), argument) {
			PHALCON_CALL_METHOD(&argument_expression, getThis(), "getsqlexpression", argument, escape_char);
			phalcon_array_append(sql_arguments, argument_expression, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		PHALCON_INIT_VAR(arguments_joined);
		phalcon_fast_join_str(arguments_joined, SL(", "), sql_arguments);
		if (phalcon_array_isset_str(expression, SL("distinct"))) {
			PHALCON_CONCAT_VSVS(return_value, name, "(DISTINCT ", arguments_joined, ")");
		} else {
			PHALCON_CONCAT_VSVS(return_value, name, "(", arguments_joined, ")");
		}

		RETURN_MM();
	}

	PHALCON_CONCAT_VS(return_value, name, "()");
	RETURN_MM();
}

/**
 * Transform an intermediate representation for a schema/table into a database system valid expression
 *
 * @param array $table
 * @param string $escapeChar
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, getSqlTable){

	zval *table, *escape_char = NULL, *table_name, *sql_table = NULL;
	zval *schema_name, *sql_schema = NULL, *alias_name;
	zval *sql_table_alias = NULL;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &table, &escape_char);

	if (!escape_char) {
		PHALCON_INIT_VAR(escape_char);
	} else {
		PHALCON_SEPARATE_PARAM(escape_char);
	}

	if (Z_TYPE_P(escape_char) == IS_NULL) {
		escape_char = phalcon_read_property(getThis(), SL("_escapeChar"), PH_NOISY);
	}
	if (Z_TYPE_P(table) == IS_ARRAY) { 

		/**
		 * The index '0' is the table name
		 */
		PHALCON_OBS_VAR(table_name);
		phalcon_array_fetch_long(&table_name, table, 0, PH_NOISY);
		if (PHALCON_GLOBAL(db).escape_identifiers) {
			PHALCON_INIT_VAR(sql_table);
			PHALCON_CONCAT_VVV(sql_table, escape_char, table_name, escape_char);
		} else {
			PHALCON_CPY_WRT(sql_table, table_name);
		}

		/**
		 * The index '1' is the schema name
		 */
		PHALCON_OBS_VAR(schema_name);
		phalcon_array_fetch_long(&schema_name, table, 1, PH_NOISY);
		if (PHALCON_IS_NOT_EMPTY(schema_name)) {
			if (PHALCON_GLOBAL(db).escape_identifiers) {
				PHALCON_INIT_VAR(sql_schema);
				PHALCON_CONCAT_VVVSV(sql_schema, escape_char, schema_name, escape_char, ".", sql_table);
			} else {
				PHALCON_INIT_NVAR(sql_schema);
				PHALCON_CONCAT_VSV(sql_schema, schema_name, ".", sql_table);
			}
		} else {
			PHALCON_CPY_WRT(sql_schema, sql_table);
		}

		/**
		 * The index '2' is the table alias
		 */
		if (phalcon_array_isset_long(table, 2)) {

			PHALCON_OBS_VAR(alias_name);
			phalcon_array_fetch_long(&alias_name, table, 2, PH_NOISY);
			if (PHALCON_GLOBAL(db).escape_identifiers) {
				PHALCON_INIT_VAR(sql_table_alias);
				PHALCON_CONCAT_VSVVV(sql_table_alias, sql_schema, " AS ", escape_char, alias_name, escape_char);
			} else {
				PHALCON_INIT_NVAR(sql_table_alias);
				PHALCON_CONCAT_VSV(sql_table_alias, sql_schema, " AS ", alias_name);
			}
		} else {
			PHALCON_CPY_WRT(sql_table_alias, sql_schema);
		}

		RETURN_CTOR(sql_table_alias);
	}

	if (PHALCON_GLOBAL(db).escape_identifiers) {
		PHALCON_CONCAT_VVV(return_value, escape_char, table, escape_char);
		RETURN_MM();
	}

	RETURN_CTOR(table);
}

/**
 * Builds a SELECT statement
 *
 * @param array $definition
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, select){

	zval *definition, escape_char, columns, selected_columns, *distinct;
	zval *column = NULL, columns_sql, tables, selected_tables;
	zval tables_sql, *sql, *joins;
	zval *join = NULL, where_conditions;
	zval *where_expression = NULL, *group_items, group_fields;
	zval *group_field = NULL, group_sql, group_clause, having_conditions, *having_expression = NULL;
	zval order_fields, order_items, *order_item = NULL;
	zval order_sql, tmp1, tmp2, limit_value, number, offset;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &definition);

	if (Z_TYPE_P(definition) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid SELECT definition");
		return;
	}
	if (!phalcon_array_isset_str(definition, SL("tables"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'tables' is required in the definition array");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&columns, definition, SL("columns"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'columns' is required in the definition array");
		return;
	}

	if (PHALCON_GLOBAL(db).escape_identifiers) {
		 phalcon_return_property(&escape_char, getThis(), SL("_escapeChar"));
	}

	if (Z_TYPE_P(&columns) == IS_ARRAY) { 
		array_init(&selected_columns);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&columns), column) {
			zval column_item, column_sql, column_domain, column_domain_sql, column_alias, column_alias_sql;
			/**
			 * Escape column name
			 */
			if (
				    phalcon_array_isset_fetch_long(&column_item, column, 0)
				 || phalcon_array_isset_fetch_str(&column_item, column, SL("column"))
			) {
				if (Z_TYPE_P(&column_item) == IS_ARRAY) {
					PHALCON_CALL_METHOD(&column_sql, getThis(), "getsqlexpression", &column_item, &escape_char);
				} else if (PHALCON_IS_STRING(column_item, "*")) {
					ZVAL_COPY_VALUE(&column_sql, &column_item);
				} else if (PHALCON_GLOBAL(db).escape_identifiers) {
					PHALCON_CONCAT_VVV(&column_sql, &escape_char, &column_item, &escape_char);
				} else {
					ZVAL_COPY_VALUE(&column_sql, &column_item);
				}
			} else {
				PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid SELECT definition");
				return;
			}

			/**
			 * Escape column domain
			 */
			if (phalcon_array_isset_fetch_long(&column_domain, column, 1)) {
				if (zend_is_true(&column_domain)) {
					if (PHALCON_GLOBAL(db).escape_identifiers) {
						PHALCON_CONCAT_VVVSV(&column_domain_sql, &escape_char, &column_domain, &escape_char, ".", &column_sql);
					} else {
						PHALCON_CONCAT_VSV(&column_domain_sql, &column_domain, ".", &column_sql);
					}
				} else {
					ZVAL_COPY_VALUE(&column_domain_sql, &column_sql);
				}
			} else {
				ZVAL_COPY_VALUE(&column_domain_sql, &column_sql);
			}

			/**
			 * Escape column alias
			 */
			if (phalcon_array_isset_fetch_long(&column_alias, column, 2)) {
				if (zend_is_true(&column_alias)) {
					if (PHALCON_GLOBAL(db).escape_identifiers) {
						PHALCON_CONCAT_VSVVV(&column_alias_sql, &column_domain_sql, " AS ", &escape_char, &column_alias, &escape_char);
					} else {
						PHALCON_CONCAT_VSV(&column_alias_sql, &column_domain_sql, " AS ", &column_alias);
					}
				} else {
					ZVAL_COPY_VALUE(&column_alias_sql,& &column_domain_sql);
				}
			} else {
				ZVAL_COPY_VALUE(&column_alias_sql, &column_domain_sql);
			}

			phalcon_array_append(&selected_columns, &column_alias_sql, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_fast_join_str(&columns_sql, SL(", "), &selected_columns);
	} else {
		ZVAL_COPY_VALUE(&columns_sql, &columns);
	}

	/**
	 * Check and escape tables
	 */
	phalcon_array_fetch_str(&tables, definition, SL("tables"), PH_NOISY);
	if (Z_TYPE_P(&tables) == IS_ARRAY) { 
		array_init(&selected_tables);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&tables), table) {
			zval *sql_table = NULL;
			PHALCON_CALL_METHOD(&sql_table, getThis(), "getsqltable", table, &escape_char);
			phalcon_array_append(&selected_tables, sql_table, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_fast_join_str(&tables_sql, SL(", "), &selected_tables);
	} else {
		ZVAL_COPY_VALUE(&tables_sql, &tables);
	}

	if (phalcon_array_isset_fetch_str(&distinct, definition, SL("distinct"))) {
		if (Z_TYPE_P(distinct) == IS_LONG) {
			if (Z_LVAL_P(distinct) == 0) {
				ZVAL_STRING(sql, "SELECT ALL ");
			} else if (Z_LVAL_P(distinct) == 1) {
				ZVAL_STRING(&sql, "SELECT DISTINCT ");
			} else {
				ZVAL_STRING(&sql, "SELECT ");
			}
		} else {
			ZVAL_STRING(&sql, "SELECT ");
		}
	} else {
		ZVAL_STRING(&sql, "SELECT ");
	}

	PHALCON_SCONCAT_VSV(&sql, &columns_sql, " FROM ", &tables_sql);

	/**
	 * Check for joins
	 */
	if (phalcon_array_isset_fetch_str(&joins, definition, SL("joins"))) {
		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&joins), join) {
			zval type, table, *sql_table = NULL, sql_join, join_conditions_array;
			zval *join_condition, join_expressions, join_conditions;

			phalcon_array_fetch_str(&type, join, SL("type"), PH_NOISY);
			phalcon_array_fetch_str(&table, join, SL("source"), PH_NOISY);

			PHALCON_CALL_METHOD(&sql_table, getThis(), "getsqltable", &table, &escape_char);

			phalcon_array_append(&selected_tables, sql_table, PH_COPY);

			PHALCON_CONCAT_SVSV(&sql_join, " ", &type, " JOIN ", sql_table);

			/**
			 * Check if the join has conditions
			 */
			if (phalcon_array_isset_fetch_str(&join_conditions_array, join, SL("conditions"))) {
				if (phalcon_fast_count_ev(join_conditions_array)) {
					array_init(&join_expressions);

					ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&join_conditions_array), join_condition) {
						zval *join_expression = NULL;
						PHALCON_CALL_METHOD(&join_expression, getThis(), "getsqlexpression", join_condition, &escape_char);
						phalcon_array_append(join_expressions, join_expression, PH_COPY);
					} ZEND_HASH_FOREACH_END();

					phalcon_fast_join_str(&join_conditions, SL(" AND "), &join_expressions);
					PHALCON_SCONCAT_SVS(&sql_join, " ON ", join_conditions, " ");
				}
			}
			phalcon_concat_self(&sql, &sql_join);
		} ZEND_HASH_FOREACH_END();

	}

	/* Check for a WHERE clause */
	if (phalcon_array_isset_fetch_str(&where_conditions, definition, SL("where"))) {
		if (Z_TYPE_P(&where_conditions) == IS_ARRAY) { 
			PHALCON_CALL_METHOD(&where_expression, getThis(), "getsqlexpression", &where_conditions, &escape_char);
			PHALCON_SCONCAT_SV(&sql, " WHERE ", where_expression);
		} else {
			PHALCON_SCONCAT_SV(&sql, " WHERE ", &where_conditions);
		}
	}

	/* Check for a GROUP clause */
	if (phalcon_array_isset_fetch_str(&group_fields, definition, SL("group"))) {
		array_init(&group_items);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&group_fields), group_field) {
			zval *group_expression = NULL;
			PHALCON_CALL_METHOD(&group_expression, getThis(), "getsqlexpression", group_field, &escape_char);
			phalcon_array_append(&group_items, group_expression, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_fast_join_str(&group_sql, SL(", "), &group_items);

		PHALCON_CONCAT_SV(&group_clause, " GROUP BY ", &group_sql);
		phalcon_concat_self(&sql, &group_clause);
	}

	/* Check for a HAVING clause */
	if (phalcon_array_isset_fetch_str(&having_conditions, definition, SL("having"))) {
		PHALCON_CALL_METHOD(&having_expression, getThis(), "getsqlexpression", &having_conditions, &escape_char);
		PHALCON_SCONCAT_SV(&sql, " HAVING ", having_expression);
	}

	/* Check for a ORDER clause */
	if (phalcon_array_isset_fetch_str(&order_fields, definition, SL("order"))) {
		array_init(&order_items);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&order_fields), order_item) {
			zval order_expression, *order_sql_item = NULL, sql_order_type, order_sql_item_type;

			phalcon_array_fetch_long(&order_expression, order_item, 0, PH_NOISY);

			PHALCON_CALL_METHOD(&order_sql_item, getThis(), "getsqlexpression", &order_expression, &escape_char);

			/**
			 * In the numeric 1 position could be a ASC/DESC clause
			 */
			if (phalcon_array_isset_long(order_item, 1)) {
				phalcon_array_fetch_long(&sql_order_type, order_item, 1, PH_NOISY);
				PHALCON_CONCAT_VSV(&order_sql_item_type, order_sql_item, " ", &sql_order_type);
			} else {
				ZVAL_COPY_VALUE(&order_sql_item_type, order_sql_item);
			}

			phalcon_array_append(&order_items, &order_sql_item_type, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_fast_join_str(&order_sql, SL(", "), &order_items);
		PHALCON_SCONCAT_SV(&sql, " ORDER BY ", &order_sql);
	}

	/**
	 * Check for a LIMIT condition
	 */
	if (phalcon_array_isset_fetch_str(&limit_value, definition, SL("limit"))) {
		if (likely(Z_TYPE_P(&limit_value) == IS_ARRAY)) {
			if (likely(phalcon_array_isset_fetch_str(&number, limit_value, SL("number")))) {
				phalcon_array_fetch_str(&tmp1, number, SL("value"), PH_NOISY);

				/**
				 * Check for a OFFSET condition
				 */
				if (phalcon_array_isset_fetch_str(&offset, limit_value, SL("offset"))) {
					phalcon_array_fetch_str(&tmp2, offset, SL("value"), PH_NOISY);
					PHALCON_SCONCAT_SVSV(&sql, " LIMIT ", &tmp1, " OFFSET ", &tmp2);
				} else {
					PHALCON_SCONCAT_SV(&sql, " LIMIT ", &tmp1);
				}
			}
		} else {
			PHALCON_SCONCAT_SV(&sql, " LIMIT ", &limit_value);
		}
	}

	/**
	 * Check for a FOR UPDATE clause
	 */
	if (phalcon_array_isset_str(definition, SL("forupdate"))) {
		PHALCON_RETURN_CALL_METHOD(getThis(), "forupdate", &sql);
		RETURN_MM();
	}

	RETURN_CTOR(sql);
}



/**
 * Builds a INSERT statement
 *
 * @param array $definition
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, insert){
	zval *definition, table, fields, values, exception_message;
	zval *escaped_table = NULL, *escape_char = NULL, joined_values, escaped_fields;
	zval *field = NULL, *joined_fields;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &definition);

	if (Z_TYPE_P(definition) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid INSERT definition");
		return;
	}
	if (!phalcon_array_isset_fetch_str(&table, definition, SL("table"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'table' is required in the definition array");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&fields, definition, SL("fields"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'fields' is required in the definition array");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&values, definition, SL("values"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'values' is required in the definition array");
		return;
	}

	if (unlikely(Z_TYPE_P(values) != IS_ARRAY)) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The second parameter for insert isn't an Array");
		return;
	}

	/**
	 * A valid array with more than one element is required
	 */
	if (!phalcon_fast_count_ev(&values)) {
		PHALCON_CONCAT_SVS(&exception_message, "Unable to insert into ", &table, " without data");
		PHALCON_THROW_EXCEPTION_ZVAL(phalcon_db_exception_ce, &exception_message);
		return;
	}

	PHALCON_CALL_METHOD(&escaped_table, getThis(), "getsqltable", &table);

	PHALCON_CALL_METHOD(&escape_char, getThis(), "getescapechar");

	/**
	 * Build the final SQL INSERT statement
	 */
	phalcon_fast_join_str(&joined_values, SL(", "), values);
	if (Z_TYPE_P(&fields) == IS_ARRAY) { 
		if (PHALCON_GLOBAL(db).escape_identifiers) {
			array_init(&escaped_fields);

			ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&fields), field) {
				zval escaped_field;

				PHALCON_CONCAT_VVV(&escaped_field, escape_char, field, escape_char);
				phalcon_array_append(escaped_fields, &escaped_field, PH_COPY);
			} ZEND_HASH_FOREACH_END();

		} else {
			ZVAL_COPY_VALUE(&escaped_fields, &fields);
		}

		phalcon_fast_join_str(&joined_fields, SL(", "), escaped_fields);

		PHALCON_CONCAT_SVSVSVS(return_value, "INSERT INTO ", escaped_table, " (", &joined_fields, ") VALUES (", joined_values, ")");
	} else {
		PHALCON_CONCAT_SVSVS(return_value, "INSERT INTO ", escaped_table, " VALUES (", &joined_values, ")");
	}

	RETURN_MM();
}

/**
 * Builds a UPDATE statement
 *
 * @param array $definition
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, update){

	zval *definition, *quoting = NULL, tables, fields, values, escape_char;
	zval updated_tables, *table = NULL, tables_sql, sql;
	zval updated_fields, *column = NULL, columns_sql;
	zval where_conditions, *where_expression = NULL;
	zval order_fields, order_items, *order_item = NULL, order_sql;
	zval limit_value, number, offset, tmp1, tmp2;
	zend_string *str_key;
	ulong idx;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 1, &definition, &quoting);

	if (Z_TYPE_P(definition) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid Update definition");
		return;
	}

	if (!quoting) {
		quoting = &PHALCON_GLOBAL(z_false);
	}

	if (!phalcon_array_isset_fetch_str(&tables, definition, SL("tables"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'tables' is required in the definition array");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&fields, definition, SL("fields"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'fields' is required in the definition array");
		return;
	}

	if (!phalcon_array_isset_fetch_str(&values, definition, SL("values"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'fields' is required in the definition array");
		return;
	}

	if (PHALCON_GLOBAL(db).escape_identifiers) {
		phalcon_return_property(&escape_char, getThis(), SL("_escapeChar"));
	}

	/**
	 * Check and escape tables
	 */
	array_init(&updated_tables);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(tables), table) {
		zval *table_expression = NULL;
		PHALCON_CALL_METHOD(&table_expression, getThis(), "getsqltable", table, escape_char);
		phalcon_array_append(&updated_tables, table_expression, PH_COPY);
	} ZEND_HASH_FOREACH_END();

	phalcon_fast_join_str(&tables_sql, SL(", "), &updated_tables);

	PHALCON_SCONCAT_SV(&sql, "UPDATE ", &tables_sql);

	array_init(&updated_fields);

	ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(fields), idx, str_key, column) {
		zval position, column_name, value_expr, value, *value_expression = NULL, column_expression;

		if (str_key) {
			ZVAL_STR(&position, str_key);
		} else {
			ZVAL_LONG(&position, idx);
		}
		if (Z_TYPE_P(column) != IS_ARRAY) {
			PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid UPDATE definition");
			return;
		}

		phalcon_array_fetch_str(&column_name, column, SL("name"), PH_NOISY);
		phalcon_array_fetch(&value_expr, values, &position, PH_NOISY);
		phalcon_array_fetch_str(&value, value_expr, SL("value"), PH_NOISY);

		PHALCON_CALL_METHOD(&value_expression, getThis(), "getsqlexpression", &value, &escape_char, quoting);

		PHALCON_CONCAT_VSV(&column_expression, &column_name, " = ", value_expression);

		phalcon_array_append(&updated_fields, &column_expression, PH_COPY);
	} ZEND_HASH_FOREACH_END();

	phalcon_fast_join_str(&columns_sql, SL(", "), &updated_fields);

	PHALCON_SCONCAT_SV(&sql, " SET ", columns_sql);

	/* Check for a WHERE clause */
	if (phalcon_array_isset_fetch_str(&where_conditions, definition, SL("where"))) {
		if (Z_TYPE_P(where_conditions) == IS_ARRAY) { 
			PHALCON_CALL_METHOD(&where_expression, getThis(), "getsqlexpression", &where_conditions, &escape_char);
			PHALCON_SCONCAT_SV(&sql, " WHERE ", where_expression);
		} else {
			PHALCON_SCONCAT_SV(&sql, " WHERE ", &where_conditions);
		}
	}

	/* Check for a ORDER clause */
	if (phalcon_array_isset_fetch_str(&order_fields, definition, SL("order"))) {	
		array_init(&order_items);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&order_fields), order_item) {
			zval order_expression, *order_sql_item = NULL, sql_order_type, order_sql_item_type;

			phalcon_array_fetch_long(&order_expression, order_item, 0, PH_NOISY);

			PHALCON_CALL_METHOD(&order_sql_item, getThis(), "getsqlexpression", &order_expression, &escape_char);

			/**
			 * In the numeric 1 position could be a ASC/DESC clause
			 */
			if (phalcon_array_isset_fetch_long(&sql_order_type, order_item, 1)) {
				PHALCON_CONCAT_VSV(&order_sql_item_type, order_sql_item, " ", &sql_order_type);
			} else {
				ZVAL_COPY_VALUE(&order_sql_item_type, order_sql_item);
			}
			phalcon_array_append(&order_items, &order_sql_item_type, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_fast_join_str(&order_sql, SL(", "), &order_items);
		PHALCON_SCONCAT_SV(&sql, " ORDER BY ", &order_sql);
	}

	/**
	 * Check for a LIMIT condition
	 */
	if (phalcon_array_isset_fetch_str(&limit_value, definition, SL("limit"))) {
		if (likely(Z_TYPE_P(&limit_value) == IS_ARRAY)) {
			if (likely(phalcon_array_isset_fetch_str(&number, &limit_value, SL("number")))) {
				phalcon_array_fetch_str(&tmp1, &number, SL("value"), PH_NOISY);

				/**
				 * Check for a OFFSET condition
				 */
				if (phalcon_array_isset_fetch_str(&offset, &limit_value, SL("offset"))) {
					phalcon_array_fetch_str(&tmp2, &offset, SL("value"), PH_NOISY);
					PHALCON_SCONCAT_SVSV(&sql, " LIMIT ", &tmp1, " OFFSET ", &tmp2);
				} else {
					PHALCON_SCONCAT_SV(&sql, " LIMIT ", &tmp1);
				}
			}
		} else {
			PHALCON_SCONCAT_SV(&sql, " LIMIT ", &limit_value);
		}
	}

	RETURN_CTOR(&sql);
}

/**
 * Builds a DELETE statement
 *
 * @param array $definition
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, delete){

	zval *definition, tables, escape_char;
	zval updated_tables, *table = NULL, tables_sql, sql;
	zval where_conditions, *where_expression = NULL;
	zval order_fields, *order_items, *order_item = NULL, order_sql;
	zval limit_value, number, offset, tmp1, tmp2;

	PHALCON_MM_GROW();

	phalcon_fetch_params(1, 1, 0, &definition);

	if (Z_TYPE_P(definition) != IS_ARRAY) { 
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "Invalid Update definition");
		return;
	}
	if (!phalcon_array_isset_fetch_str(&tables, definition, SL("tables"))) {
		PHALCON_THROW_EXCEPTION_STR(phalcon_db_exception_ce, "The index 'tables' is required in the definition array");
		return;
	}

	if (PHALCON_GLOBAL(db).escape_identifiers) {
		phalcon_return_property(&escape_char, getThis(), SL("_escapeChar"));
	}

	/**
	 * Check and escape tables
	 */
	array_init(updated_tables);

	ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(&tables), table) {
		zval *sql_table = NULL;
		PHALCON_CALL_METHOD(&sql_table, getThis(), "getsqltable", table, &escape_char);
		phalcon_array_append(&updated_tables, sql_table, PH_COPY);
	} ZEND_HASH_FOREACH_END();

	phalcon_fast_join_str(&tables_sql, SL(", "), &updated_tables);

	PHALCON_SCONCAT_SV(&sql, "DELETE FROM ", &tables_sql);

	/* Check for a WHERE clause */
	if (phalcon_array_isset_fetch_str(&where_conditions, definition, SL("where"))) {
		if (Z_TYPE_P(&where_conditions) == IS_ARRAY) { 
			PHALCON_CALL_METHOD(&where_expression, getThis(), "getsqlexpression", &where_conditions, &escape_char);
			PHALCON_SCONCAT_SV(&sql, " WHERE ", where_expression);
		} else {
			PHALCON_SCONCAT_SV(&sql, " WHERE ", &where_conditions);
		}
	}

	/* Check for a ORDER clause */
	if (phalcon_array_isset_fetch_str(&order_fields, definition, SL("order"))) {
		array_init(&order_items);

		ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(order_fields), order_item) {
			zval order_expression, *order_sql_item = NULL, sql_order_type, order_sql_item_type;

			phalcon_array_fetch_long(&order_expression, order_item, 0, PH_NOISY);

			PHALCON_CALL_METHOD(&order_sql_item, getThis(), "getsqlexpression", &order_expression, &escape_char);

			/**
			 * In the numeric 1 position could be a ASC/DESC clause
			 */
			if (phalcon_array_isset_long(order_item, 1)) {
				phalcon_array_fetch_long(&sql_order_type, order_item, 1, PH_NOISY);
				PHALCON_CONCAT_VSV(&order_sql_item_type, order_sql_item, " ", &sql_order_type);
			} else {
				ZVAL_COPY_VALUE(&order_sql_item_type, order_sql_item);
			}
			phalcon_array_append(&order_items, &order_sql_item_type, PH_COPY);
		} ZEND_HASH_FOREACH_END();

		phalcon_fast_join_str(&order_sql, SL(", "), &order_items);
		PHALCON_SCONCAT_SV(sql, " ORDER BY ", &order_sql);
	}

	/**
	 * Check for a LIMIT condition
	 */
	if (phalcon_array_isset_fetch_str(&limit_value, definition, SL("limit"))) {
		if (likely(Z_TYPE_P(&limit_value) == IS_ARRAY)) {
			if (likely(phalcon_array_isset_fetch_str(&number, &limit_value, SL("number")))) {
				phalcon_array_fetch_str(&tmp1, &number, SL("value"), PH_NOISY);

				/**
				 * Check for a OFFSET condition
				 */
				if (phalcon_array_isset_fetch_str(&offset, &limit_value, SL("offset"))) {
					phalcon_array_fetch_str(&tmp2, &offset, SL("value"), PH_NOISY);
					PHALCON_SCONCAT_SVSV(&sql, " LIMIT ", &tmp1, " OFFSET ", &tmp2);
				} else {
					PHALCON_SCONCAT_SV(&sql, " LIMIT ", &tmp1);
				}
			}
		} else {
			PHALCON_SCONCAT_SV(&sql, " LIMIT ", &limit_value);
		}
	}

	RETURN_CTOR(&sql);
}

/**
 * Checks whether the platform supports savepoints
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Db_Dialect, supportsSavepoints){


	RETURN_TRUE;
}

/**
 * Checks whether the platform supports releasing savepoints.
 *
 * @return boolean
 */
PHP_METHOD(Phalcon_Db_Dialect, supportsReleaseSavepoints)
{
	PHALCON_RETURN_CALL_METHODW(getThis(), "supportssavepoints");
}

/**
 * Generate SQL to create a new savepoint
 *
 * @param string $name
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, createSavepoint){

	zval *name;

	phalcon_fetch_params(0, 1, 0, &name);

	PHALCON_CONCAT_SV(return_value, "SAVEPOINT ", name);
	return;
}

/**
 * Generate SQL to release a savepoint
 *
 * @param string $name
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, releaseSavepoint){

	zval *name;

	phalcon_fetch_params(0, 1, 0, &name);

	PHALCON_CONCAT_SV(return_value, "RELEASE SAVEPOINT ", name);
	return;
}

/**
 * Generate SQL to rollback a savepoint
 *
 * @param string $name
 * @return string
 */
PHP_METHOD(Phalcon_Db_Dialect, rollbackSavepoint){

	zval *name;

	phalcon_fetch_params(0, 1, 0, &name);

	PHALCON_CONCAT_SV(return_value, "ROLLBACK TO SAVEPOINT ", name);
	return;
}

/**
 * Registers custom SQL functions
 *
 * @return array
 */
PHP_METHOD(Phalcon_Db_Dialect, registerCustomFunction){

	zval *name, *custom_function;

	phalcon_fetch_params(0, 2, 0, &name, &custom_function);

	phalcon_update_property_array(getThis(), SL("_customFunctions"), name, custom_function);

	RETURN_THISW();
}

/**
 * Returns registered functions
 *
 * @return array
 */
PHP_METHOD(Phalcon_Db_Dialect, getCustomFunctions){


	RETURN_MEMBER(getThis(), "_customFunctions");
}
