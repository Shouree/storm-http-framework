#include "stdafx.h"
#include "Tokenizer.h"
#include "Exception.h"
#include "Utils/TextReader.h"
#include "Utils/FileStream.h"


/**
 * Utils.
 */

static const wchar_t *operators = L"+?*&=.:!";
static const wchar_t *specials = L"[](){},-;<>~";

static bool isOperator(wchar_t c) {
	for (const wchar_t *p = operators; *p; p++)
		if (c == *p)
			return true;
	return false;
}

static bool isSpecial(wchar_t c) {
	for (const wchar_t *p = specials; *p; p++)
		if (c == *p)
			return true;
	return false;
}

static bool isWhitespace(wchar_t c) {
	switch (c) {
	case ' ':
	case '\n':
	case '\r':
	case '\t':
		return true;
	}
	return false;
}

static void advance(SrcPos &pos, const String &src, nat from, nat to) {
	// for (nat i = from; i < to; i++) {
	// 	if (src[i] == '\n') {
	// 		pos.line++;
	// 		pos.col = 0;
	// 	} else {
	// 		pos.col++;
	// 	}
	// }
	for (nat i = from; i < to; i++)
		if (src[i] != '\r')
			pos.pos++;
}


/**
 * Token.
 */

wostream &operator <<(wostream &to, const Token &token) {
	return to << token.token;
}

bool Token::isStr() const {
	return token.size() >= 2
		&& token[0] == '"'
		&& token[token.size()-1] == '"';
}

String Token::strVal() const {
	assert(isStr());
	return token.substr(1, token.size() - 2);
}


/**
 * Comment.
 */

wostream &operator <<(wostream &to, const Comment &comment) {
	return to << comment.str();
}

String Comment::str() const {
	std::wostringstream r;
	State state = start;
	Params par = { 0, 0, 0 };

	for (nat i = begin; i < end; i++) {
		if (const wchar_t *msg = parse(r, state, par, (*src)[i])) {
			SrcPos pos(fileId, 0);
			advance(pos, *src, 0, i);
			throw Error(L"Malformed comment: " + ::toS(msg), pos);
		}
	}

	return r.str();
}

const wchar_t *Comment::parse(std::wostringstream &r, State &state, Params &par, wchar_t ch) {
	// Just ignore any CR. We only want LF in the final output.
	if (ch == '\r')
		return null;

	switch (state) {
	case start:
		// Always a '/' that should be ignored.
		if (ch != '/')
			return L"Inconsistent comment. Internal error?";

		state = start2;
		return null;
	case start2:
		// Either '*' or '/'.
		if (ch == '*')
			state = multiStart;
		else if (ch == '/')
			state = singleStart;
		else
			return L"Unknown comment type. Internal error?";
		return null;
	case done:
		// Nothing more to do, just ignore the rest. (TODO: Assert on additional data?)
		return null;

		/**
		 * Single-line comments.
		 */

	case singleStart:
		// Find out the number of spaces before the comment starts.
		if (isWhitespace(ch)) {
			par.space++;
		} else {
			r << ch;
			state = singleInside;
		}
		return null;

	case singleInside:
		// Inside of a singleline comment.
		if (ch == '\n') {
			par.curr = 0;
			par.empty = 1;
			state = singleNewline;
		} else {
			r << ch;
		}
		return null;

	case singleNewline:
		// After a newline, before the first asterisk.
		if (ch == '/')
			state = singleHalf;
		else if (!isWhitespace(ch))
			return L"Expected // before the comment text.";
		return null;

	case singleHalf:
		// After the first '/'.
		if (ch == '/')
			state = singleBefore;
		else
			return L"Expected // before the comment text.";
		return null;

	case singleBefore:
		// Right after the second /. Count number of spaces and emit any text we find.
		if (ch == ' ') {
			par.curr++;
		} else if (ch == '\n') {
			par.empty++;
			par.curr = 0;
			state = singleNewline;
		} else if (!isWhitespace(ch)) {
			while (par.empty) {
				r << '\n';
				par.empty--;
			}

			if (par.curr < par.space)
				return L"Not enough indentation after //";
			for (nat i = par.space; i < par.curr; i++)
				r << ' ';
			par.curr = 0;

			r << ch;
			state = singleInside;
		}
		return null;


		/**
		 * Multi-line comments.
		 */

	case multiStart:
		// Skip remaining asterisks and compute the number of spaces to ignore after each asterisk.
		if (ch == '*') {
			par.space = 0;
		} else if (isWhitespace(ch)) {
			par.space++;
		} else {
			r << ch;
			state = multiInside;
		}
		return null;

	case multiInside:
		// Inside of a multiline comment.
		if (ch == '\n') {
			par.curr = 0;
			par.empty = 1;
			state = multiNewline;
		} else {
			r << ch;
		}
		return null;

	case multiNewline:
		// After a newline, before the first asterisk.
		if (ch == '*')
			state = multiBefore;
		else if (!isWhitespace(ch))
			return L"Expected at least one asterisk before the comment text.";
		return null;

	case multiBefore:
		// Right after an asterisk. Count number of spaces and emit any text we find.
		if (par.curr == 0 && ch == '/') {
			state = done;
		} else if (ch == ' ') {
			par.curr++;
		} else if (ch == '\n') {
			par.empty++;
			par.curr = 0;
			state = multiNewline;
		} else if (ch == '*') {
			// The comment pads on LHS with asterisks, we allow that since that is what is done in SQLite.
			state = multiBefore;
		} else if (!isWhitespace(ch)) {
			while (par.empty) {
				r << '\n';
				par.empty--;
			}

			if (par.curr < par.space)
				return L"Not enough indentation after the first asterisk.";
			for (nat i = par.space; i < par.curr; i++)
				r << ' ';
			par.curr = 0;

			r << ch;
			state = multiInside;
		}
		return null;
	}

	return L"Internal error: This state is not recognized.";
}


/**
 * Tokenizer.
 */

Tokenizer::Tokenizer(nat fileId) :
	src(readTextFile(SrcPos::files[fileId])),
	pathId(fileId), pos(0), srcPos(fileId, 0),
	commentBegin(0), commentEnd(0),
	nextToken(L"", SrcPos()) {

	nextToken = findNext();
}

Token Tokenizer::next() {
	Token t = peek();
	nextToken = findNext();
	return t;
}

void Tokenizer::skip() {
	nextToken = findNext();
}

Token Tokenizer::peek() {
	if (!more())
		throw Error(L"End of file!", srcPos);

	return nextToken;
}

void Tokenizer::expect(const String &t) {
	Token tok = next();
	if (tok.token != t)
		throw Error(L"Expected " + t + L" but got " + tok.token + L".", tok.pos);
}

bool Tokenizer::skipIf(const String &t) {
	Token tok = peek();
	if (tok.token == t) {
		skip();
		return true;
	} else {
		return false;
	}
}

Comment Tokenizer::comment() const {
	return Comment(src, srcPos.fileId, commentBegin, commentEnd);
}

void Tokenizer::clearComment() {
	commentBegin = commentEnd = 0;
}

bool Tokenizer::more() const {
	return !nextToken.empty();
}

void Tokenizer::advance(SrcPos &pos, nat from, nat to) const {
	::advance(pos, src, from, to);
}

Token Tokenizer::findNext() {
	State state = sStart;
	nat start = pos;
	nat from = pos; // from may be modified, while start may not.
	bool firstComment = true;
	SrcPos sStart = srcPos;

	while (state != sDone) {
		processChar(from, state, firstComment);
	}

	// Advance line-counting.
	advance(sStart, start, from);
	advance(srcPos, start, pos);

	return Token(src.substr(from, pos - from), sStart);
}

void Tokenizer::processChar(nat &start, State &state, bool &firstComment) {
	if (pos >= src.size()) {
		state = sDone;
		return;
	}

	wchar_t ch = src[pos];

	if (ch == '/' && pos+1 < src.size() && (src[pos+1] == '/' || src[pos+1] == '*')) {
		switch (state) {
		case sStart:
			if (src[pos+1] == '*') {
				state = sMlComment;
				commentBegin = pos;
			} else {
				state = sComment;
				if (firstComment) {
					commentBegin = pos;
					firstComment = false;
				}
			}
			break;
		case sString:
		case sComment:
		case sMlComment:
		case sPreproc:
			break;
		default:
			state = sDone;
			return;
		}
	}

	switch (state) {
	case sStart:
		pos++;
		if (ch == '\n') {
			// Do not count single-line comments that have a blank line inside them as a whole comment block.
			start = pos;
			firstComment = true;
		} else if (isWhitespace(ch)) {
			start = pos;
		} else if (isSpecial(ch)) {
			state = sDone;
		} else if (isOperator(ch)) {
			state = sOperator;
		} else if (ch == '"') {
			state = sString;
		} else if (ch == '#') {
			firstComment = true;
			state = sPreproc;
		} else {
			state = sText;
		}
		break;
	case sText:
		if (isOperator(ch) || isWhitespace(ch) || isSpecial(ch) || ch == '"') {
			state = sDone;
		} else {
			pos++;
		}
		break;
	case sOperator:
		if (!isOperator(ch)) {
			state = sDone;
		} else {
			pos++;
		}
		break;
	case sString:
		pos++;
		if (ch == '"') {
			state = sDone;
		} else if (ch == '\\') {
			pos++;
		}
		break;
	case sComment:
		start = ++pos;
		if (ch == '\n')
			state = sStart;
		else if (ch != '\r')
			commentEnd = start;
		break;
	case sMlComment:
		start = ++pos;
		if (ch == '*' && pos < src.size() && src[pos] == '/') {
			pos++;
			state = sStart;
		} else {
			commentEnd = start - 1;
		}
		break;
	case sPreproc:
		start = ++pos;
		if (ch == '\\') {
			state = sPreprocExtend;
		} else if (ch == '\n') {
			state = sStart;
		}
		break;
	case sPreprocExtend:
		start = ++pos;
		if (ch == '\n')
			state = sPreproc;
		else if (ch == '\r')
			;
		else
			state = sPreproc;
		break;
	case sDone:
		// pos--;
		break;
	}
}

