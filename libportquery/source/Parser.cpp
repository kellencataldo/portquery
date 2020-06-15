#include "Parser.h"

#include <iostream>

// helper class for std::visit. Constructs a callable which accepts various types based on deduction
template<class ... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


template<typename T> bool MATCH(const Token t) { return std::holds_alternative<T>(t); }


std::string getTokenString(const Token t) {

    return std::visit(overloaded {
                [=] (NumericToken n)  { return std::to_string(n.m_value); },
                [=] (ComparisonToken) { return std::string("[COMPARISON TOKEN]"); },
                [=] (UserToken u)     { return u.m_UserToken; },
                [=] (ALLToken)        { return std::string("[ALL KEYWORD]"); },
                [=] (ANDToken)        { return std::string("AND KEYWORD]"); },
                [=] (ANYToken)        { return std::string("[ANY KEYWORD]"); },
                [=] (BETWEENToken)    { return std::string("[BETWEEN KEYWORD]"); },
                [=] (COUNTToken)      { return std::string("[COUNT KEYWORD]"); },
                [=] (FROMToken)       { return std::string("[FROM KEYWORD]"); },
                [=] (IFToken)         { return std::string("[IF KEYWORD]"); },
                [=] (INToken)         { return std::string("[IN KEYWORD]"); },
                [=] (ISToken)         { return std::string("[IS KEYWORD]"); },
                [=] (LIKEToken)       { return std::string("[LIKE KEYWORD]"); },
                [=] (LIMITToken)      { return std::string("[LIMIT KEYWORD]"); },
                [=] (NOTToken)        { return std::string("[NOT KEYWORD]"); },
                [=] (ORToken)         { return std::string("[OR KEYWORD]"); },
                [=] (ORDERToken)      { return std::string("[ORDER KEYWORD]"); },
                [=] (SELECTToken)     { return std::string("[SELECT KEYWORD]");},
                [=] (WHEREToken)      { return std::string("[WHERE KEYWORD]"); },
                [=] (PORTToken)       { return std::string("[PORT  KEYWORD]"); },
                [=] (ProtocolToken)   { return std::string("[PROTOCOL TOKEN]"); }, // more granularity please
                [=] (EOFToken)        { return std::string("[END OF INPUT]"); },
                [=] (PunctuationToken<'*'>) { return std::string("[ * ]"); },
                [=] (PunctuationToken<'('>) { return std::string("[ ( ]"); }, 
                [=] (PunctuationToken<')'>) { return std::string("[ ) ]"); },
                [=] (PunctuationToken<';'>) { return std::string("[ ; ]"); }, 
                [=] (PunctuationToken<','>) { return std::string("[ , ]"); },
                [=] (auto) -> std::string { return std::string("[UNKNOWN TOKEN]"); } }, 
            t);
    }


SOSQLSelectStatement Parser::parseSOSQLStatement() {

    // SOSQL statements can obviously only begin with the "SELECT" keyword
    if (!MATCH<SELECTToken>(m_lexer.nextToken())) {

        throw std::invalid_argument("Only SELECT statements are handled. Statement must begin with SELECT");
    }


    const auto [selectPort, selectedProtocols] = parseSelectSetQuantifier();
    const std::string tableReference = parseTableReference();
    const SOSQLExpression tableExpression = parseTableExpression();

    // parse end here, check for EOF and semicolon
    // parse where statement here.
    return { selectPort, selectedProtocols, tableReference, tableExpression };
}


SOSQLExpression Parser::parseTableExpression() {


    if (!MATCH<WHEREToken>(m_lexer.peek())) {

        return std::make_shared<NULLExpression>();
    }

    m_lexer.nextToken(); // this is the WHERE token;
    return parseORExpression();
}


SOSQLExpression Parser::parseORExpression() {

    SOSQLExpression expression = parseANDExpression();
    while (MATCH<ORToken>(m_lexer.peek())) {

        m_lexer.nextToken(); // scan past or token
        SOSQLExpression right = parseANDExpression();
        expression = std::make_shared<ORExpression>(ORExpression{expression, right});
    }


    return expression;
}


SOSQLExpression Parser::parseANDExpression() {

    SOSQLExpression expression = parseBooleanFactor();
    while (MATCH<ANDToken>(m_lexer.peek())) {

        m_lexer.nextToken(); // scan past and token
        SOSQLExpression right = parseBooleanFactor();
        expression = std::make_shared<ANDExpression>(ANDExpression{expression, right});
    }


    return expression;
}


SOSQLExpression Parser::parseBooleanFactor() {

    if(MATCH<NOTToken>(m_lexer.peek())) {
        m_lexer.nextToken();
        return std::make_shared<NOTExpression>(NOTExpression{parseBooleanExpression()});
    }

    return parseBooleanFactor();
}


SOSQLExpression Parser::parseBooleanExpression() {

    const Token lhs = m_lexer.nextToken();
    if (!MATCH<ProtocolToken>(lhs) && !MATCH<PORTToken>(lhs) && !MATCH<NumericToken>(lhs)) {

        throw std::invalid_argument("Invalid token type specified in expression: " + getTokenString(lhs));
    }

    else if (!MATCH<PORTToken>(lhs) && MATCH<BETWEENToken>(m_lexer.peek())) {

        throw std::invalid_argument("Only the PORT column can be used in a BETWEEN expression");
    }

    return std::visit(overloaded {
            [=] (ComparisonToken) { return parseComparisonExpression(lhs); },
            [=] (ISToken)             { return parseISExpression(lhs); },
            [=] (BETWEENToken) {  return parseBETWEENExpression(lhs); }, 
            [=] (auto const t) -> SOSQLExpression {
                const std::string exceptionString = "Invalid operator token in expression: " + getTokenString(t);
                throw std::invalid_argument(exceptionString); 
            } },
        m_lexer.nextToken());
}


SOSQLExpression Parser::parseComparisonExpression(const Token lhs) {
    UNUSED_PARAMETER(lhs);
    return NULL;
}


SOSQLExpression Parser::parseISExpression(const Token lhs) {
    UNUSED_PARAMETER(lhs);
    return NULL;
}


SOSQLExpression Parser::parseBETWEENExpression(const Token lhs) {
    UNUSED_PARAMETER(lhs);
    return NULL;
}

std::tuple<bool, NetworkProtocols> Parser::parseSelectSetQuantifier() {

    return std::visit(overloaded {
            [=] (ProtocolToken)         { return parseSelectList(); },
            [=] (PORTToken)             { return parseSelectList(); },
            [=] (PunctuationToken<'*'>) { 
                m_lexer.nextToken(); 
                return std::tuple<bool, NetworkProtocols>{true, (NetworkProtocols::TCP|NetworkProtocols::UDP)}; },
            [=] (auto t) -> std::tuple<bool, NetworkProtocols> {
                const std::string exceptionString = "Invalid token in select list: " + getTokenString(t);
                throw std::invalid_argument(exceptionString); 
            } },
        m_lexer.peek());

};

std::tuple<bool, NetworkProtocols> Parser::parseSelectList() {
    
    NetworkProtocols selectedProtocols = NetworkProtocols::NONE;
    bool selectPort = false;

    bool moreColumns = true;
    while (moreColumns) {
        std::visit(overloaded { 
                [&selectPort] (PORTToken) { 
                    if (selectPort) { throw std::invalid_argument("Duplicate PORT column specified"); }
                    selectPort = true; 
                },

                [&selectedProtocols] (const ProtocolToken p) {
                    if (NetworkProtocols::NONE != (p.m_protocol & selectedProtocols)) { 
                        throw std::invalid_argument("Duplicate protocol column specified"); }
                    selectedProtocols |= p.m_protocol;
                },

                [=] (const auto t) -> void { 
                    std::string exceptionString = "Invalid token specified in select list: " + getTokenString(t);
                    throw std::invalid_argument(exceptionString);
                } },
           
            m_lexer.nextToken());

        // If there is a comma here, there are more columns to scan
        moreColumns = MATCH<PunctuationToken<','>>(m_lexer.peek());
        if(moreColumns) {
            m_lexer.nextToken();
        }
    }

    return { selectPort, selectedProtocols };
}


std::string Parser::parseTableReference() {

    const Token t = m_lexer.nextToken();
    if (!MATCH<FROMToken>(t)) {
        const std::string exceptionString = "FROM token not following column list, invalid token specified: " + getTokenString(t);
        throw std::invalid_argument(exceptionString);
    }

    return std::visit(overloaded {
            [=] (UserToken u)  { return u.m_UserToken; },
            [=] (auto t) -> std::string { 
                throw std::invalid_argument("Invalid token following FROM keyword: " + getTokenString(t)); } 
            }, 
        m_lexer.nextToken());
}
