#pragma once
#include <vector>
#include <initializer_list>

namespace RegularExp {

	class Expression {
	public:
		virtual int Check(char*& current, char* end) const = 0;
	};

	/*
		Wildcard, detects any one character as a pattern
	*/
	class Any : public Expression {
	public:
		Any() {};
		~Any() {};

		virtual int Check(char*& current, char* end) const override;
	};

	/*
		Detect a pattern where one Expression is found atleast the number of times specified by "min",
			and a maximum number of times specified by "max".

		Example: Star_Min_Max(exp, 0, 10) -> exp must be found between 0 and 10 times
		Example: Star_Min_Max(exp, 5, 10) -> exp must be found between 5 and 10 times
		Example: Star_Min_Max(exp, 5, -1) -> exp must be found atleast 5 times
		Example: Star_Min_Max(exp, 5, 5)  -> exp must be found exactly 5 times
	*/
	class Star_Min_Max : public Expression {
	public:
		Star_Min_Max(const Expression& exp, unsigned int min = 0, unsigned int max = -1);
		~Star_Min_Max() {};

		virtual int Check(char*& current, char* end) const override;
	private:
		const Expression& m_exp;
		const unsigned int m_min;
		const unsigned int m_max;
	};

	/*
		Detect a pattern where one Expression is found any number of times, zero number of times included.
	*/
	class Star : public Star_Min_Max {
	public:
		Star(const Expression& exp);
		~Star() {};
	};

	/*
		Detect a pattern where one Expression is found any number of times but atleast found one time.
	*/
	class StarAtleastOne : public Star_Min_Max {
	public:
		StarAtleastOne(const Expression& exp);
		~StarAtleastOne() {};
	};

	/*
		Detect one pattern from a set of multiple Expressions.
		Only one Expressions must be true for a pattern to be found, Expressions are evaluated from left to right.
		The evaluation is stopped as soon as one pattern is found.
	*/
	class OR : public Expression {
	public:
		OR(const std::initializer_list<const Expression*> args);
		~OR() {};

		virtual int Check(char*& current, char* end) const override;
	private:
		const std::vector<const Expression*> m_args;
	};

	/*
		Detect a pattern constructed of multiple Expressions.
		All Expressions specifed must be true from left to right for a pattern to be found.
	*/
	class AND : public Expression {
	public:
		AND(const std::initializer_list<const Expression*> args);
		~AND() {};

		virtual int Check(char*& current, char* end) const override;
	private:
		const std::vector<const Expression*> m_args;
	};

	/*
		Detect any single character from a specified character Set.
	*/
	class CharacterSelect : public Expression {
	public:
		CharacterSelect(const char* characterSet);
		~CharacterSelect() {};

		virtual int Check(char*& current, char* end) const override;
	private:
		const char* m_characterSet;
	};

	/*
		Detect any specific term.
		The whole term will need to be found.
	*/
	class Term : public Expression {
	public:
		Term(const char* term);
		~Term() {};
		virtual int Check(char*& current, char* end) const override;
	private:
		const char* m_term;
	};

	/////////////////

	/*
		Detect Any Token.
	*/
	const RegularExp::Any             g_any;
	/*
		Detect [b or B]
	*/
	const RegularExp::CharacterSelect g_bB("bB");
	/*
		Detect [x or X]
	*/
	const RegularExp::CharacterSelect g_xX("xX");
	/*
		Detect [0]
	*/
	const RegularExp::CharacterSelect g_digZero("0");
	/*
		Detect any single digit from the Binary base, [0-1]
	*/
	const RegularExp::CharacterSelect g_binaryDig("01");
	/*
		Detect any single nonzero digit from the decimal base, [1-9]
	*/
	const RegularExp::CharacterSelect g_digNonZero("123456789");
	/*
		Detect any single digit from the HexaDecimal base, [0-9], [a-f] or [A-F]
	*/
	const RegularExp::CharacterSelect g_hexDig("0123456789ABCDEFabcdef");
	/*
		Detect any single Upper Letter in the range [A-Z]
	*/
	const RegularExp::CharacterSelect g_LetterUpper("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
	/*
		Detect any single Lower Letter in the range [a-z]
	*/
	const RegularExp::CharacterSelect g_LetterLower("abcdefghijklmnopqrstqvwxyz");
	/*
		Detect any single Letter in the range [a-z] or [A-Z]
	*/
	const RegularExp::OR              g_letter({ &g_LetterLower, &g_LetterUpper });
	/*
		Detect [-]
	*/
	const RegularExp::CharacterSelect g_negativeSign("-");
	/*
		Detect [-], zero one one number of times
	*/
	const RegularExp::Star_Min_Max    g_negativeSignOptional(g_negativeSign, 0, 1);
	/*
		Detect any single digit in the decimal base [0-9]
		Example: 0
		Example: 9
	*/
	const RegularExp::OR              g_digit({ &g_digNonZero, &g_digZero });
	/*
		Detect any number of digits from the Decimal base within range [0-9].
	*/
	const RegularExp::Star            g_digitStar(g_digit);
	/*
		Detect any positive number described with digits from the decimal base that do not start with zero.
	*/
	const RegularExp::AND             g_numberPositive({ &g_digNonZero, &g_digitStar });
	/*
		Detect any positive or negative number described with digits from the decimal base that do not start with zero.
	*/
	const RegularExp::AND             g_number({ &g_negativeSignOptional, &g_numberPositive });
	/*
		Detect pattern usualy found in the start of a hex number "0x" or "0X"
	*/
	const RegularExp::AND             g_hexStart({ &g_digZero, &g_xX });
	/*
		Detect any number described with digits from the HexaDecimal base, [0-9][a-f][A-F]

		Example: 9Aa4f3D...
		Example: ad10a4f...
	*/
	const RegularExp::Star_Min_Max    g_hex_number(g_hexDig, 1);
	/*
		Detect Hex numbers starting with "0x" or "0X"

		Example: 0x9Aa4f3D...
		Example: 0Xad10a4f...
	*/
	const RegularExp::AND             g_HexNumberFormated({ &g_hexStart, &g_hex_number });
	/*
		Detect pattern usualy found in the start of a binary number "0b" or "0B"
	*/
	const RegularExp::AND             g_binaryStart({ &g_digZero, &g_bB });
	/*
		Detect any number described with digits from the HexaDecimal base, [0-9][a-f][A-F]

		Example: 100110...
		Example: 001101...
	*/
	const RegularExp::Star_Min_Max    g_binaryNummber(g_binaryDig, 1);
	/*
		Detect Bynary numbers starting with "0x" or "0X"

		Example: 0b1001011...
		Example: 0B0011101...
	*/
	const RegularExp::AND             g_BinaryNumberFormated({ &g_binaryStart, &g_binaryNummber });
}
