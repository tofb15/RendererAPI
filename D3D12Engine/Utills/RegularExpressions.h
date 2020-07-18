#pragma once
#include <vector>
#include <initializer_list>

namespace RegularExp {

	class Expression {
	public:
		virtual int Check(char*& current, void* tree = nullptr) const = 0;
	};

	/*
		Detect any single character from a specified character Set.
	*/
	class CharacterSelect_Set : virtual public Expression {
	public:
		CharacterSelect_Set(const char* characterSet);
		~CharacterSelect_Set() {};

		virtual int Check(char*& current, void* tree) const override;
	private:
		const char* m_characterSet;
	};

	/*
		Detect any single character from a specified continues character range.
	*/
	class CharacterSelect_Range : virtual public Expression {
	public:
		CharacterSelect_Range(const char start, const char end);
		~CharacterSelect_Range() {};

		virtual int Check(char*& current, void* tree) const override;
	private:
		const char start;
		const char end;
	};

	/*
		Detect any single character that is not contained in a specified character Set.
	*/
	class AnyExept_Set : public CharacterSelect_Set {
	public:
		AnyExept_Set(const char* characterSet);
		~AnyExept_Set() {};
		virtual int Check(char*& current, void* tree) const override;
	};

	/*
		Wildcard, detects any one character as a pattern
	*/
	class Any : public AnyExept_Set {
	public:
		Any();
		~Any() {};

		//virtual int Check(char*& current, void* tree) const override;
	};

	/*
		Detect a pattern where one Expression is found atleast the number of times specified by "min",
			and a maximum number of times specified by "max".

		Example: Star_Min_Max(exp, 0, 10) -> exp must be found between 0 and 10 times
		Example: Star_Min_Max(exp, 5, 10) -> exp must be found between 5 and 10 times
		Example: Star_Min_Max(exp, 5, -1) -> exp must be found atleast 5 times
		Example: Star_Min_Max(exp, 5, 5)  -> exp must be found exactly 5 times
	*/
	class Star_Min_Max : virtual public Expression {
	public:
		Star_Min_Max(const Expression& exp, unsigned int min = 0, unsigned int max = -1);
		~Star_Min_Max() {};

		virtual int Check(char*& current, void* tree) const override;
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
	class OR : virtual public Expression {
	public:
		OR(const std::initializer_list<const Expression*> args);
		~OR() {};

		virtual int Check(char*& current, void* tree) const override;
	private:
		const std::vector<const Expression*> m_args;
	};

	/*
		Detect a pattern constructed of multiple Expressions.
		All Expressions specifed must be true from left to right for a pattern to be found.
	*/
	class AND : virtual public Expression {
	public:
		AND(const std::initializer_list<const Expression*> args);
		~AND() {};

		virtual int Check(char*& current, void* tree) const override;
	private:
		const std::vector<const Expression*> m_args;
	};


	/*
		Detect any specific term.
		The whole term will need to be found.
	*/
	class Term : virtual public Expression {
	public:
		Term(const char* term);
		~Term() {};
		virtual int Check(char*& current, void* tree) const override;
	private:
		const char* m_term;
	};

	/////////////////

	namespace CharacterCollections {
		/*
			Detect Any Character.
		*/
		const RegularExp::Any             g_any;

		/*
			Detect any single Upper Letter in the range [A-Z]
		*/
		const RegularExp::CharacterSelect_Range g_LetterUpper('A', 'Z');

		/*
			Detect any single Lower Letter in the range [a-z]
		*/
		const RegularExp::CharacterSelect_Range g_LetterLower('a', 'z');

		/*
			Detect any single Letter in the range [a-z] or [A-Z]
		*/
		const RegularExp::OR g_letter({ &g_LetterLower, &g_LetterUpper });

		/*
			Detect [0]
		*/
		const RegularExp::CharacterSelect_Set g_digZero("0");
		/*
			Detect any single digit from the Binary base, [0-1]
		*/
		const RegularExp::CharacterSelect_Set g_binaryDig("01");
		/*
			Detect any single nonzero digit from the decimal base, [1-9]
		*/
		const RegularExp::CharacterSelect_Range g_digNonZero('1', '9');

		/*
			Detect any single digit in the decimal base [0-9]
			Example: 0
			Example: 9
		*/
		const RegularExp::OR              g_digit({ &g_digNonZero, &g_digZero });

		/*
			Detect any single digit from the HexaDecimal base, [0-9], [a-f] or [A-F]
		*/
		const RegularExp::OR g_hexDig({ &g_digit, &g_letter });
		/*
			Detect a single letter or digit

			Example: A
			Example: b
			Example: 5
		*/
		const RegularExp::OR              g_LetterOrDigit({ &g_letter, &RegularExp::CharacterCollections::g_digit });
		/*
			Detect a pattern of mixed letters or digits.
			Empty string are detected as a pattern.

			Example: AbcDe12Fg...
			Example: 1helloWorld1...
		*/
		const RegularExp::Star            g_LetterOrDigit_Star({ g_LetterOrDigit });

	}

	namespace Special {
		/*
			Detect [-]
		*/
		const RegularExp::CharacterSelect_Set g_negativeSign("-");

		const RegularExp::CharacterSelect_Set g_seperator_token(" \t");
		const RegularExp::Star            g_seperator_star(g_seperator_token);
		const RegularExp::StarAtleastOne  g_seperator_atleastOne(g_seperator_token);
		const RegularExp::CharacterSelect_Set g_EOL_token("\n");
		//const RegularExp::OR			  g_EOL_EOF({ &g_EOL_token, &g_EOF_token });
		//
		const RegularExp::AnyExept_Set        g_not_EOL("\n");
		//const RegularExp::AND			  g_not_EOL_EOF({ &g_not_EOL, &g_not_EOF });
		const RegularExp::Star			  g_not_EOL_star(g_not_EOL);

		const RegularExp::CharacterSelect_Set g_hashtag_token("#");
		const RegularExp::CharacterSelect_Set g_equal_token("=");
		const RegularExp::CharacterSelect_Set g_dot(".");

	}
	namespace Numbers {
		/*
			Detect [-], zero one one number of times
		*/
		const RegularExp::Star_Min_Max    g_negativeSignOptional(Special::g_negativeSign, 0, 1);

		/*
			Detect [b or B]
		*/
		const RegularExp::CharacterSelect_Set g_bB("bB");

		/*
			Detect [x or X]
		*/
		const RegularExp::CharacterSelect_Set g_xX("xX");

		/*
			Detect pattern usualy found in the start of a hex number "0x" or "0X"
		*/
		const RegularExp::AND             g_hexStart({ &CharacterCollections::g_digZero, &g_xX });

		/*
			Detect pattern usualy found in the start of a binary number "0b" or "0B"
		*/
		const RegularExp::AND             g_binaryStart({ &CharacterCollections::g_digZero, &g_bB });

		/*
			Detect any sequence of digits, zero included, from the Decimal base within range [0-9].
		*/
		const RegularExp::Star            g_digitStar(CharacterCollections::g_digit);
		/*
			Detect any positive number described with digits from the decimal base that do not start with zero.
		*/
		const RegularExp::AND             g_numberPositive({ &CharacterCollections::g_digNonZero, &g_digitStar });
		/*
			Detect any positive or negative number described with digits from the decimal base that do not start with zero.
		*/
		const RegularExp::AND             g_number({ &g_negativeSignOptional, &g_numberPositive });
		/*
			Detect any number described with digits from the Binary base, [0-1]

			Example: 100110...
			Example: 001101...
		*/
		const RegularExp::Star_Min_Max    g_binaryNummber(CharacterCollections::g_binaryDig, 1);
		/*
			Detect Binary numbers starting with "0b" or "0B"

			Example: 0b1001011...
			Example: 0B0011101...
		*/
		const RegularExp::AND             g_BinaryNumberFormated({ &g_binaryStart, &g_binaryNummber });

		/*
			Detect any number, atleast one, described with digits from the HexaDecimal base, [0-9][a-f][A-F]

			Example: 9Aa4f3D...
			Example: ad10a4f...
		*/
		const RegularExp::Star_Min_Max    g_hex_number(CharacterCollections::g_hexDig, 1);
		/*
			Detect Hex numbers starting with "0x" or "0X"

			Example: 0x9Aa4f3D...
			Example: 0Xad10a4f...
		*/
		const RegularExp::AND             g_HexNumberFormated({ &g_hexStart, &g_hex_number });
	}
	namespace Strings {
		/*
			Detect a string pattern starting with a letter followed by a mix of letters and numbers.
			Empty string are not detected as a pattern.

			Example: AbcDe12Fg...
			Example: helloWorld1...
		*/
		const RegularExp::AND             g_string_letters_digits({ &CharacterCollections::g_letter, &CharacterCollections::g_LetterOrDigit_Star });

		/*String*/
		const RegularExp::CharacterSelect_Set _g_string_special_tokens("/_\\.");
		const RegularExp::OR              _g_string_part1({ &RegularExp::CharacterCollections::g_letter, &_g_string_special_tokens });
		const RegularExp::OR              _g_string_part2({ &_g_string_part1, &RegularExp::CharacterCollections::g_digit });
		const RegularExp::Star            _g_string_part2_star(_g_string_part2);
		const RegularExp::AND             g_string_letters_tokens_digits({ &_g_string_part1, &_g_string_part2_star });

		/*File Paths*/
		//const RegularExp::CharacterSelect reg_path_seperator_startEnd("\\/");
		//const RegularExp::StarAtleastOne  reg_path_seperator_atleast_one(reg_path_seperator_startEnd);
		//const RegularExp::AND             reg_fileExtension({ &RegularExp::Special::g_dot, &reg_string });
		//const RegularExp::Star_Min_Max    reg_specialFolderName(RegularExp::Special::g_dot, 2, 2);
		//const RegularExp::OR   reg_FolderName({ &reg_string, &reg_specialFolderName });
		//const RegularExp::AND  reg_FolderPath({ &reg_FolderName, &reg_path_seperator_atleast_one });
		//const RegularExp::Star reg_FolderName_star(reg_FolderPath);
		//const RegularExp::AND  reg_full_Path({ &reg_FolderName_star, &reg_string, &reg_fileExtension });
	}
}
