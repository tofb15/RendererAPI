#pragma once

namespace RegularExp {
	class Expression {
	public:
		virtual int Check(char* current, char* end) = 0;
	};

	class Any : public Expression
	{
	public:
		Any() {};
		~Any() {};

		virtual int Check(char* current, char* end) override {
			if (current != end) {
				current++;
				return 1;
			}
			else {
				return -1;
			}
		};
	};

	class Star : public Expression
	{
	public:
		Star(Expression& exp) : m_exp(exp) {};
		~Star() {};

		virtual int Check(char* current, char* end) override {
			while (m_exp.Check(current, end) > 0)
			{

			}
		};
	private:
		Expression& m_exp;
	};

	class CharacterSelect : public Expression
	{
	public:
		CharacterSelect(const char* characterSet) : m_characterSet(characterSet) {};
		~CharacterSelect() {};

		virtual int Check(char* current, char* end) override {
			int i = 0;
			const char* start_temp = current;
			while (m_characterSet[i] != '\0')
			{
				if (current[0] == m_characterSet[i++]) {
					current++;
					i = 0;
				}
			}

			if (start_temp == current) {
				return -1;
			}
			else {
				return current - start_temp;
			}
		};
	private:
		const char* m_characterSet;
	};

	class Digit : public CharacterSelect
	{
	public:
		Digit() : CharacterSelect("0123456789") {};
		~Digit() {};
	};

	class DigitZero : public CharacterSelect
	{
	public:
		DigitZero() : CharacterSelect("0") {};
		~DigitZero() {};
	};

	class DigitNonZero : public CharacterSelect
	{
	public:
		DigitNonZero() : CharacterSelect("123456789") {};
		~DigitNonZero() {};
	};
}
