/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements a functional test for the Verhoeff objects.
 *
 */

#ifdef VERHOEFF_TEST

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Verhoeff.h"

class VerhoeffTest
{
public:
	VerhoeffTest();

	uint64_t duration;
	int TestCount_SDE;
	int SuccessCount_SDE;
	int TestCount_ATE;
	int SuccessCount_ATE;
	int TestCount_JTE;
	int SuccessCount_JTE;

	void Reset();

	template<class Verhoeff>
	void RunTests(int strLen, int division, int divisions, bool runBasicTests = true);

	template<class Verhoeff>
	void SearchPermutations(int strLen, int division, int divisions);

private:
	template<class Verhoeff>
	char *MakeTestStrings(int strLen, int division, int divisions);

	template<class Verhoeff>
	void MakeTestString(char *buf, int strLen, int val);

	template<class Verhoeff>
	void TestSingleDigitErrors(char *str, size_t strLen);

	template<class Verhoeff>
	void TestAdjacentTranspositions(char *str, size_t strLen);

	template<class Verhoeff>
	void TestJumpTranspositions(char *str, size_t strLen);

	template<class Verhoeff>
	void PermutePermutationTable();

	static int RandRange(int mod);

	static inline uint64_t Now()
	{
#ifdef WIN32
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		return (((uint64_t)ft.dwHighDateTime) << 32 | ft.dwLowDateTime) / 10000;
#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
	}
};

VerhoeffTest::VerhoeffTest()
{
	Reset();
}

void VerhoeffTest::Reset()
{
	duration = 0;
	TestCount_SDE = 0;
	SuccessCount_SDE = 0;
	TestCount_ATE = 0;
	SuccessCount_ATE = 0;
	TestCount_JTE = 0;
	SuccessCount_JTE = 0;
}

template <class Verhoeff>
void VerhoeffTest::RunTests(int strLen, int division, int divisions, bool runBasicTests)
{
	Reset();

	char *testStrings = MakeTestStrings<Verhoeff>(strLen, division, divisions);

	uint64_t start = Now();

	char *curStr = testStrings;
	while (*curStr != 0)
	{
		size_t len = strlen(curStr);

		if (runBasicTests)
		{
			TestSingleDigitErrors<Verhoeff>(curStr, len);
			TestAdjacentTranspositions<Verhoeff>(curStr, len);
		}

		TestJumpTranspositions<Verhoeff>(curStr, len);

		curStr += len + 1;
	}

	duration = Now() - start;

	free(testStrings);
}

template<class Verhoeff>
void VerhoeffTest::SearchPermutations(int strLen, int division, int divisions)
{
	char *str = (char *)malloc(strLen + 1);
	int sdeFails = 0;
	int ateFails = 0;
	int bestJTE = -1;

	srand(time(NULL));

	int totalStrs = Verhoeff::Base;
	for (int i = 2; i < strLen; i++)
		totalStrs *= Verhoeff::Base;

	int startVal = (totalStrs * division) / divisions;
	int endVal = (totalStrs * (division + 1)) / divisions;


	for (int permCount = 0; true; permCount++)
	{
		if (permCount != 0 && (permCount % 100000) == 0)
			printf("%d complete (%d, %d)\n", permCount, sdeFails, ateFails);

		Reset();

		for (int val = startVal; val < endVal; val++)
		{
			MakeTestString<Verhoeff>(str, strLen, val);

#if WEAVE_CONFIG_VERHOEFF_TEST_SINGLE_DIGIT_ERRORS
			TestSingleDigitErrors<Verhoeff>(str, strLen);
			if (SuccessCount_SDE != TestCount_SDE)
			{
				sdeFails++;
				goto next_perm;
			}
#endif // WEAVE_CONFIG_VERHOEFF_TEST_SINGLE_DIGIT_ERRORS

			TestAdjacentTranspositions<Verhoeff>(str, strLen);
			if (SuccessCount_ATE != TestCount_ATE)
			{
				ateFails++;
				goto next_perm;
			}

			TestJumpTranspositions<Verhoeff>(str, strLen);
		}

		if (SuccessCount_JTE > bestJTE)
		{
			bestJTE = SuccessCount_JTE;

			printf("%u/%u : ", SuccessCount_JTE, TestCount_JTE);
			for (int i = 0; i < Verhoeff::Base; i++)
				printf("%2u, ", (unsigned)Verhoeff::sPermTable[i]);
			printf("\n");
		}

	next_perm:

		PermutePermutationTable<Verhoeff>();
	}
}

template <class Verhoeff>
char *VerhoeffTest::MakeTestStrings(int strLen, int division, int divisions)
{
	if (strLen < 2)
		return NULL;

	int totalStrs = Verhoeff::Base;
	for (int i = 2; i < strLen; i++)
		totalStrs *= Verhoeff::Base;

	int startVal = (totalStrs * division) / divisions;
	int endVal = (totalStrs * (division + 1)) / divisions;

	char *strTable = (char *)malloc((strLen + 1) * (endVal - startVal) + 1);

	char *curStr = strTable;
	for (int val = startVal; val < endVal; val++, curStr += strLen + 1)
		MakeTestString<Verhoeff>(curStr, strLen, val);

	// zero-length string marks end of table
	*curStr = 0;

	return strTable;
}

template <class Verhoeff>
void VerhoeffTest::MakeTestString(char *buf, int strLen, int val)
{
	int x = val;
	for (int i = strLen - 2; i >= 0; i--)
	{
		int charVal = x % Verhoeff::Base;
		buf[i] = Verhoeff::ValToChar(charVal);
		x = x / Verhoeff::Base;
	}
	buf[strLen-1] = Verhoeff::ComputeCheckChar(buf, strLen - 1);
	buf[strLen] = 0;
}

template <class Verhoeff>
void VerhoeffTest::TestSingleDigitErrors(char *str, size_t strLen)
{
	char *p = str;

	while (*p)
	{
		char origChar = *p;
		int origVal = Verhoeff::CharToVal(origChar);
		int testVal = origVal;

		while (true)
		{
			testVal++;
			if (testVal == Verhoeff::Base)
				testVal = 0;
			if (testVal == origVal)
				break;
			*p = Verhoeff::ValToChar(testVal);
			TestCount_SDE++;
			if (!Verhoeff::ValidateCheckChar(str, strLen))
				SuccessCount_SDE++;
		}

		*p = origChar;
		p++;
	}
}

template <class Verhoeff>
void VerhoeffTest::TestAdjacentTranspositions(char *str, size_t strLen)
{
	for (size_t i = 1; i < strLen; i++)
	{
		char a = str[i-1], b = str[i];

		if (a == b)
			continue;

		str[i-1] = b;
		str[i] = a;

		TestCount_ATE++;
		if (!Verhoeff::ValidateCheckChar(str, strLen))
			SuccessCount_ATE++;

		str[i-1] = a;
		str[i] = b;
	}
}

template <class Verhoeff>
void VerhoeffTest::TestJumpTranspositions(char *str, size_t strLen)
{
	for (size_t i = 2; i < strLen; i++)
	{
		char a = str[i-2], b = str[i];

		if (a == b)
			continue;

		str[i-2] = b;
		str[i] = a;

		TestCount_JTE++;
		if (!Verhoeff::ValidateCheckChar(str, strLen))
			SuccessCount_JTE++;

		str[i-2] = a;
		str[i] = b;
	}
}

template<class Verhoeff>
void VerhoeffTest::PermutePermutationTable()
{
	for (int i = Verhoeff::Base - 1; i > 0; i--)
	{
		int rnd = RandRange(i + 1);
		uint8_t swap = Verhoeff::sPermTable[i];
		Verhoeff::sPermTable[i] = Verhoeff::sPermTable[rnd];
		Verhoeff::sPermTable[rnd] = swap;
	}
}

int VerhoeffTest::RandRange(int limit)
{
	int max = RAND_MAX - (RAND_MAX % limit);
	int rnd;
	do
		rnd = rand();
	while (rnd >= max);
	return rnd % limit;
}


extern "C" int main(int argc, char *argv[])
{
	typedef Verhoeff10 V;

	char *cmd = strrchr(argv[0], '/');
#ifdef WIN32
	if (cmd == NULL)
		cmd = strrchr(argv[0], '\\');
	if (cmd == NULL)
		cmd = strrchr(argv[0], '\\');
#endif
	if (cmd == NULL)
		cmd = argv[0];

	if (argc > 1 && strcmp(argv[1], "generate") == 0)
	{
		if (argc != 3)
		{
			fprintf(stderr, "Usage: %s generate <string>\n", cmd);
			return -1;
		}

		char checkChar = V::ComputeCheckChar(argv[2]);
		if (checkChar == 0)
		{
			fprintf(stderr, "Invalid character in input\n");
			return -1;
		}

		printf("%s%c\n", argv[2], checkChar);
	}

	else if (argc > 1 && strcmp(argv[1], "verify") == 0)
	{
		if (argc != 3)
		{
			fprintf(stderr, "Usage: %s verify <string>\n", cmd);
			return -1;
		}

		if (V::ValidateCheckChar(argv[2]))
		{
			printf("%s is VALID\n", argv[2]);
			return 0;
		}
		else
		{
			printf("%s is INVALID\n", argv[2]);
			return -1;
		}
	}

	else if (argc > 1 && strcmp(argv[1], "test") == 0)
	{
		VerhoeffTest test;

		if (argc != 3)
		{
			fprintf(stderr, "Usage: %s test <base(base-10/base-16/base-32/base-36)>\n", cmd);
			return -1;
		}

		const char *testStr = "123456";
		char verifyStr[10] = "";
		char checkChar = '0';

		// determin Verhoeff Base and run tests
		if (strcmp(argv[2], "base-10") == 0)
		{
			printf("testing Verhoeff10\n");

			// run a simple compute and validate test
			checkChar = Verhoeff10::ComputeCheckChar(testStr);
			sprintf(verifyStr, "%s%c", testStr, checkChar);
			if (!Verhoeff10::ValidateCheckChar(verifyStr) ||
				!Verhoeff10::ValidateCheckChar(checkChar, testStr))
			{
				printf("%s is INVALID\n", verifyStr);
				return -1;
			}

			// run fully Verhoeff tests
			test.RunTests<Verhoeff10>(6, 0, 1000, true);
		}
		else if (strcmp(argv[2], "base-16") == 0)
		{
			printf("testing Verhoeff16\n");

			// run a simple compute and validate test
			checkChar = Verhoeff16::ComputeCheckChar(testStr);
			sprintf(verifyStr, "%s%c", testStr, checkChar);
			if (!Verhoeff16::ValidateCheckChar(verifyStr) ||
				!Verhoeff16::ValidateCheckChar(checkChar, testStr))
			{
				printf("%s is INVALID\n", verifyStr);
				return -1;
			}

			// run fully Verhoeff tests
			test.RunTests<Verhoeff16>(6, 0, 1000, true);
		}
		else if (strcmp(argv[2], "base-32") == 0)
		{
			printf("testing Verhoeff32\n");

			// run a simple compute and validate test
			checkChar = Verhoeff32::ComputeCheckChar(testStr);
			sprintf(verifyStr, "%s%c", testStr, checkChar);
			if (!Verhoeff32::ValidateCheckChar(verifyStr) ||
				!Verhoeff32::ValidateCheckChar(checkChar, testStr))
			{
				printf("%s is INVALID\n", verifyStr);
				return -1;
			}

			// run fully Verhoeff tests
			test.RunTests<Verhoeff32>(6, 0, 1000, true);
		}
		else if (strcmp(argv[2], "base-36") == 0)
		{
			printf("testing Verhoeff36\n");

			// run a simple compute and validate test
			checkChar = Verhoeff36::ComputeCheckChar(testStr);
			sprintf(verifyStr, "%s%c", testStr, checkChar);
			if (!Verhoeff36::ValidateCheckChar(verifyStr) ||
				!Verhoeff36::ValidateCheckChar(checkChar, testStr))
			{
				printf("%s is INVALID\n", verifyStr);
				return -1;
			}

			// run fully Verhoeff tests
			test.RunTests<Verhoeff36>(6, 0, 1000, true);
		}
		else
		{
			printf("Invalid Verhoeff base\n");
			return -1;
		}

		printf("duration = %lu.%03lu\n", (unsigned long)(test.duration / 1000), (unsigned long)(test.duration % 1000));
		if (test.TestCount_SDE != 0)
			printf("single digit errors found = %f%%\n", (double)test.SuccessCount_SDE * 100.0 / (double)test.TestCount_SDE);
		else
			printf("no single digit errors tested\n");
		if (test.TestCount_ATE != 0)
			printf("adjacent transpositions found = %f%%\n", (double)test.SuccessCount_ATE * 100.0 / (double)test.TestCount_ATE);
		else
			printf("no adjacent transpositions tested\n");
		if (test.TestCount_JTE != 0)
			printf("jump transpositions found = %f%%\n", (double)test.SuccessCount_JTE * 100.0 / (double)test.TestCount_JTE);
		else
			printf("no jump transpositions tested\n");
	}

	else if (argc > 1 && strcmp(argv[1], "search-perms") == 0)
	{
		VerhoeffTest test;
		test.SearchPermutations<V>(3, 0, 1);
	}

	else
	{
		fprintf(stderr,
				"Usage: %s <command> [ <args> ]\n"
				"\n"
				"Commands:\n"
				"  generate <string>\n"
				"  verify <string-with-check-digit>\n"
				"  test <base(base-10/base-16/base-32/base-36)>\n"
				"  search-perms\n",
				cmd);
		return -1;
	}

	return 0;
}

#endif // VERHOEFF_TEST
