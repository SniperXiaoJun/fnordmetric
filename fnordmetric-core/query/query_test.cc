/**
 * This file is part of the "FnordStream" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * Licensed under the MIT license (see LICENSE).
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include "queryparser.h"
#include "token.h"
#include "tokenize.h"
//#include "../agent.h"
//#include "../clock.h"
//#include "../query/query.h"
//#include "../database/database.h"

namespace fnordmetric {
namespace query {

class QueryTest {
public:
  QueryTest() {}

  /*
    SELECT concat(fnord + 5, -somefunc(myotherfield)) + (123 * 4)
    SELECT -sum(fnord) + (123 * 4)
    SELECT (-blah + sum(fnord) / (123 * 4)) as myfield
    select true;
    select !(true);
    select NOT true;
  */

  void run() {
    testTokenizerSimple();
    testTokenizerEscaping();
    testTokenizerAsClause();
    testSelectMustBeFirstAssert();
    testSelectWildcard();
    testSelectTableWildcard();
    testSelectDerivedColumn();
    testSelectDerivedColumnWithTableName();
    testSimpleValueExpression();
    testNegatedValueExpression();
    testMethodCallValueExpression();
    testComplexQuery1();
  }

  QueryParser parseTestQuery(const char* query) {
    QueryParser parser;
    parser.parse(query, strlen(query));
    return parser;
  }

  void testSimpleValueExpression() {
    auto parser = parseTestQuery("SELECT 23 + 5.123 FROM sometable;");
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    assert(*stmt == ASTNode::T_SELECT);
    assert(stmt->getChildren().size() == 2);
    const auto& sl = stmt->getChildren()[0];
    assert(*sl == ASTNode::T_SELECT_LIST);
    assert(sl->getChildren().size() == 1);
    auto derived = sl->getChildren()[0];
    assert(*derived == ASTNode::T_DERIVED_COLUMN);
    assert(derived->getChildren().size() == 1);
    auto expr = derived->getChildren()[0];
    assert(*expr == ASTNode::T_ADD_EXPR);
    assert(expr->getChildren().size() == 2);
    assert(*expr->getChildren()[0] == ASTNode::T_LITERAL);
    assert(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
    assert(*expr->getChildren()[0]->getToken() == "23");
    assert(*expr->getChildren()[1] == ASTNode::T_LITERAL);
    assert(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
    assert(*expr->getChildren()[1]->getToken() == "5.123");
    const auto& from = stmt->getChildren()[1];
    assert(*from == ASTNode::T_FROM);
  }

  void testMethodCallValueExpression() {
    auto parser = parseTestQuery("SELECT 1 + sum(23, 4 + 1) FROM sometable;");
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    assert(*stmt == ASTNode::T_SELECT);
    assert(stmt->getChildren().size() == 2);
    const auto& sl = stmt->getChildren()[0];
    assert(*sl == ASTNode::T_SELECT_LIST);
    assert(sl->getChildren().size() == 1);
    auto derived = sl->getChildren()[0];
    assert(*derived == ASTNode::T_DERIVED_COLUMN);
    assert(derived->getChildren().size() == 1);
    auto expr = derived->getChildren()[0];
    assert(*expr == ASTNode::T_ADD_EXPR);
    assert(expr->getChildren().size() == 2);
    assert(*expr->getChildren()[0] == ASTNode::T_LITERAL);
    assert(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
    assert(*expr->getChildren()[0]->getToken() == "1");
    auto mcall = expr->getChildren()[1];
    assert(*mcall == ASTNode::T_METHOD_CALL);
    assert(*mcall->getToken() == Token::T_IDENTIFIER);
    assert(*mcall->getToken() == "sum");
    assert(mcall->getChildren().size() == 2);
    assert(*mcall->getChildren()[0] == ASTNode::T_LITERAL);
    assert(*mcall->getChildren()[0]->getToken() == Token::T_NUMERIC);
    assert(*mcall->getChildren()[0]->getToken() == "23");
    assert(*mcall->getChildren()[1] == ASTNode::T_ADD_EXPR);
    assert(mcall->getChildren()[1]->getChildren().size() == 2);
    const auto& from = stmt->getChildren()[1];
    assert(*from == ASTNode::T_FROM);
  }

  void testNegatedValueExpression() {
    auto parser = parseTestQuery("SELECT -(23 + 5.123) AS fucol FROM tbl;");
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    assert(*stmt == ASTNode::T_SELECT);
    assert(stmt->getChildren().size() == 2);
    const auto& sl = stmt->getChildren()[0];
    assert(*sl == ASTNode::T_SELECT_LIST);
    assert(sl->getChildren().size() == 1);
    auto derived = sl->getChildren()[0];
    assert(*derived == ASTNode::T_DERIVED_COLUMN);
    assert(derived->getChildren().size() == 2);
    auto neg_expr = derived->getChildren()[0];
    assert(*neg_expr == ASTNode::T_NEGATE_EXPR);
    assert(neg_expr->getChildren().size() == 1);
    auto expr = neg_expr->getChildren()[0];
    assert(*expr == ASTNode::T_ADD_EXPR);
    assert(expr->getChildren().size() == 2);
    assert(*expr->getChildren()[0] == ASTNode::T_LITERAL);
    assert(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
    assert(*expr->getChildren()[0]->getToken() == "23");
    assert(*expr->getChildren()[1] == ASTNode::T_LITERAL);
    assert(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
    assert(*expr->getChildren()[1]->getToken() == "5.123");
    auto col_name = derived->getChildren()[1];
    assert(*col_name == ASTNode::T_COLUMN_NAME);
    assert(*col_name->getToken() == Token::T_IDENTIFIER);
    assert(*col_name->getToken() == "fucol");
    const auto& from = stmt->getChildren()[1];
    assert(*from == ASTNode::T_FROM);
  }

  void testSelectWildcard() {
    auto parser = parseTestQuery("SELECT * FROM sometable;");
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    assert(*stmt == ASTNode::T_SELECT);
    assert(stmt->getChildren().size() == 2);
    const auto& sl = stmt->getChildren()[0];
    assert(*sl == ASTNode::T_SELECT_LIST);
    assert(sl->getChildren().size() == 1);
    assert(*sl->getChildren()[0] == ASTNode::T_ALL);
    assert(sl->getChildren()[0]->getToken() == nullptr);
    const auto& from = stmt->getChildren()[1];
    assert(*from == ASTNode::T_FROM);
  }

  void testSelectTableWildcard() {
    auto parser = parseTestQuery("SELECT mytablex.* FROM sometable;");
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    assert(*stmt == ASTNode::T_SELECT);
    assert(stmt->getChildren().size() == 2);
    const auto& sl = stmt->getChildren()[0];
    assert(*sl == ASTNode::T_SELECT_LIST);
    assert(sl->getChildren().size() == 1);
    const auto& all = sl->getChildren()[0];
    assert(*all == ASTNode::T_ALL);
    assert(all->getToken() != nullptr);
    assert(*all->getToken() == Token::T_IDENTIFIER);
    assert(*all->getToken() == "mytablex");
    const auto& from = stmt->getChildren()[1];
    assert(*from == ASTNode::T_FROM);
  }

  void testSelectDerivedColumn() {
    auto parser = parseTestQuery("SELECT somecol AS another FROM sometable;");
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    assert(*stmt == ASTNode::T_SELECT);
    assert(stmt->getChildren().size() == 2);
    const auto& sl = stmt->getChildren()[0];
    assert(*sl == ASTNode::T_SELECT_LIST);
    assert(sl->getChildren().size() == 1);
    const auto& derived = sl->getChildren()[0];
    assert(*derived == ASTNode::T_DERIVED_COLUMN);
    assert(derived->getChildren().size() == 2);
    assert(*derived->getChildren()[0] == ASTNode::T_COLUMN_NAME);
    assert(*derived->getChildren()[0]->getToken() == Token::T_IDENTIFIER);
    assert(*derived->getChildren()[0]->getToken() == "somecol");
    assert(*derived->getChildren()[1] == ASTNode::T_COLUMN_NAME);
    assert(*derived->getChildren()[1]->getToken() == Token::T_IDENTIFIER);
    assert(*derived->getChildren()[1]->getToken() == "another");
    const auto& from = stmt->getChildren()[1];
    assert(*from == ASTNode::T_FROM);
  }

  void testSelectDerivedColumnWithTableName() {
    auto parser = parseTestQuery("SELECT tbl.col AS another FROM sometable;");
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    assert(*stmt == ASTNode::T_SELECT);
    assert(stmt->getChildren().size() == 2);
    const auto& sl = stmt->getChildren()[0];
    assert(*sl == ASTNode::T_SELECT_LIST);
    assert(sl->getChildren().size() == 1);
    const auto& derived = sl->getChildren()[0];
    assert(*derived == ASTNode::T_DERIVED_COLUMN);
    assert(derived->getChildren().size() == 2);
    auto tbl = derived->getChildren()[0];
    assert(*tbl == ASTNode::T_TABLE_NAME);
    assert(*tbl->getToken() == Token::T_IDENTIFIER);
    assert(*tbl->getToken() == "tbl");
    assert(tbl->getChildren().size() == 1);
    auto col = tbl->getChildren()[0];
    assert(*col->getToken() == Token::T_IDENTIFIER);
    assert(*col->getToken() == "col");
    assert(*derived->getChildren()[0] == ASTNode::T_TABLE_NAME);
    assert(*derived->getChildren()[0]->getToken() == Token::T_IDENTIFIER);
    assert(*derived->getChildren()[0]->getToken() == "tbl");
    assert(*derived->getChildren()[1] == ASTNode::T_COLUMN_NAME);
    assert(*derived->getChildren()[1]->getToken() == Token::T_IDENTIFIER);
    assert(*derived->getChildren()[1]->getToken() == "another");
    const auto& from = stmt->getChildren()[1];
    assert(*from == ASTNode::T_FROM);
  }

  void testSelectMustBeFirstAssert() {
    auto parser = parseTestQuery("GROUP BY SELECT");
    assert(parser.getErrors().size() == 1);
    assert(parser.getErrors()[0].type == QueryParser::ERR_UNEXPECTED_TOKEN);
  }

  void testTokenizerEscaping() {
    auto parser = parseTestQuery(" SELECT  fnord,sum(blah) from fubar blah.id"
        "= 'fnor\\'dbar' + 123.5;");
    const auto& tl = parser.token_list_;
    assert(tl.size() == 17);
    assert(tl[0].type_ == Token::T_SELECT);
    assert(tl[1].type_ == Token::T_IDENTIFIER);
    assert(tl[1] == "fnord");
    assert(tl[2].type_ == Token::T_COMMA);
    assert(tl[3].type_ == Token::T_IDENTIFIER);
    assert(tl[3] == "sum");
    assert(tl[4].type_ == Token::T_LPAREN);
    assert(tl[5].type_ == Token::T_IDENTIFIER);
    assert(tl[5] == "blah");
    assert(tl[6].type_ == Token::T_RPAREN);
    assert(tl[7].type_ == Token::T_FROM);
    assert(tl[8].type_ == Token::T_IDENTIFIER);
    assert(tl[8] == "fubar");
    assert(tl[9].type_ == Token::T_IDENTIFIER);
    assert(tl[9] == "blah");
    assert(tl[10].type_ == Token::T_DOT);
    assert(tl[11].type_ == Token::T_IDENTIFIER);
    assert(tl[11] == "id");
    assert(tl[12].type_ == Token::T_EQUAL);
    assert(tl[13].type_ == Token::T_STRING);
    //assert(tl[13] == "fnord'bar"); // FIXPAUL
    assert(tl[14].type_ == Token::T_PLUS);
    assert(tl[15].type_ == Token::T_NUMERIC);
    assert(tl[15] == "123.5");
    assert(tl[16].type_ == Token::T_SEMICOLON);
  }

  void testTokenizerSimple() {
    auto parser = parseTestQuery(" SELECT  fnord,sum(`blah-field`) from fubar"
        " blah.id= \"fn'o=,rdbar\" + 123;");
    auto tl = &parser.token_list_;
    assert((*tl)[0].type_ == Token::T_SELECT);
    assert((*tl)[1].type_ == Token::T_IDENTIFIER);
    assert((*tl)[1] == "fnord");
    assert((*tl)[2].type_ == Token::T_COMMA);
    assert((*tl)[3].type_ == Token::T_IDENTIFIER);
    assert((*tl)[3] == "sum");
    assert((*tl)[4].type_ == Token::T_LPAREN);
    assert((*tl)[5].type_ == Token::T_IDENTIFIER);
    assert((*tl)[5] == "blah-field");
    assert((*tl)[6].type_ == Token::T_RPAREN);
    assert((*tl)[7].type_ == Token::T_FROM);
    assert((*tl)[8].type_ == Token::T_IDENTIFIER);
    assert((*tl)[8] == "fubar");
    assert((*tl)[9].type_ == Token::T_IDENTIFIER);
    assert((*tl)[9] == "blah");
    assert((*tl)[10].type_ == Token::T_DOT);
    assert((*tl)[11].type_ == Token::T_IDENTIFIER);
    assert((*tl)[11] == "id");
    assert((*tl)[12].type_ == Token::T_EQUAL);
    assert((*tl)[13].type_ == Token::T_STRING);
    assert((*tl)[13] == "fn'o=,rdbar");
    assert((*tl)[14].type_ == Token::T_PLUS);
    assert((*tl)[15].type_ == Token::T_NUMERIC);
    assert((*tl)[15] == "123");
    assert((*tl)[16].type_ == Token::T_SEMICOLON);
  }

  void testTokenizerAsClause() {
    auto parser = parseTestQuery(" SELECT fnord As blah from");
    auto tl = &parser.token_list_;
    assert((*tl)[0].type_ == Token::T_SELECT);
    assert((*tl)[1].type_ == Token::T_IDENTIFIER);
    assert((*tl)[1] == "fnord");
    assert((*tl)[2].type_ == Token::T_AS);
    assert((*tl)[3].type_ == Token::T_IDENTIFIER);
    assert((*tl)[3] == "blah");
  }


  void testComplexQuery1() {
    auto parser = parseTestQuery(
      "SELECT"
      "   l_orderkey,"
      "   sum( l_extendedprice * ( 1 - l_discount) ) AS revenue,"
      "   o_orderdate,"
      "   o_shippriority"
      "FROM"
      "   customer,"
      "   orders,"
      "   lineitem"
      "WHERE"
      "  c_mktsegment = 'FURNITURE' AND"
      "  c_custkey = o_custkey AND"
      "  l_orderkey = o_orderkey AND"
      "  o_orderdate < \"2013-12-21\" AND"
      "  l_shipdate > \"2014-01-06\""
      "GROUP BY"
      "  l_orderkey,"
      "  o_orderdate,"
      "  o_shippriority"
      "ORDER BY"
      "  revenue,"
      "  o_orderdate;");

    parser.debugPrint();
    assert(parser.getErrors().size() == 0);
    assert(parser.getStatements().size() == 1);
  }

/*
  Agent getAgent() {
    auto database = fnordmetric::database::Database::openFile(
        "/tmp/__fnordmetric_testQuery",
        database::MODE_CONSERVATIVE |
        database::FILE_TRUNCATE |
        database::FILE_AUTODELETE);

    return Agent("testagent", std::move(database));
  }
*/
/*
  void testQuery() {
    auto agent = getAgent();
    auto stream = agent.openStream(
        "mystream",
        fnordmetric::IntegerField("seq"));
    auto raw_stream = agent.database_->openStream(
        stream->getKey().getKeyString());

    / * insert test records * /
    auto base_time = WallClock::getUnixMillis();
    RecordWriter record_writer(stream->getSchema());
    for (int i = 0; i < 1000; ++i) {
      record_writer.setIntegerField(0, i);
      record_writer.reset();
      raw_stream->appendRow(record_writer, base_time + i);
    }

    / * test query 1 * /
    auto results = agent.executeQuery("SELECT time, seq FROM mystream;");
  }
*/
};

}
}

int main() {
  fnordmetric::query::QueryTest test;
  test.run();
  printf("all tests passed! :)\n");
}