#include "Query.h"
#include "TextQuery.h"
#include <memory>
#include <set>
#include <algorithm>
#include <iostream>
#include <cstddef>
#include <iterator>
#include <stdexcept>
#include <regex>
using namespace std;
////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<QueryBase> QueryBase::factory(const string& s)
{
	/*if(s == "smart")
		return std::shared_ptr<QueryBase>(new WordQuery("smart"));*/

	regex check_NOT("\\s*(NOT)(\\s+\\w)\\s*");
	regex check_AND("\\s*(\\w\\s+)(AND)(\\s+\\w)\\s*");
	regex check_OR("\\s*(\\w\\s+)(OR)(\\s+\\w)\\s*");
	regex check_N("\\s*(\\w\\s+)(\\d+)(\\s+\\w)\\s*");
	std::smatch matches;

	string word1;
	string word2;

	// separate s into substrings to check for the required operation
	istringstream line(s);

	std::vector<std::string> words(std::istream_iterator<std::string>{line},
		std::istream_iterator<std::string>());

	// word1 AND word2 - search for lines containing both words
	if (std::regex_search(s, matches, check_AND))//if (words.at(1) == "AND")
	{
		word1 = words.at(0);
		word2 = words.at(2);

		return std::shared_ptr<QueryBase>(new AndQuery(word1, word2));
	}

	// word1 OR word2 - search for lines containing either word
	if (std::regex_search(s, matches, check_OR))
	{
		word1 = words.at(0);
		word2 = words.at(2);

		return std::shared_ptr<QueryBase>(new OrQuery(word1, word2));
	}

	// word1 n word2 - search for lines containing both words, with "n" (number) words between them
	if (std::regex_search(s, matches, check_N))
	{
		//cout << "check_N\n"; //DEBUG

		word1 = words.at(0);
		word2 = words.at(2);
		int dist = stoi(words.at(1));

		return std::shared_ptr<QueryBase>(new NQuery(word1, word2, dist));
	}

	// NOT
	if (std::regex_search(s, matches, check_NOT))
	{
		//cout << "check_NOT\n"; //DEBUG

		word1 = words.at(1);
		return std::shared_ptr<QueryBase>(new NotQuery(word1));
	}

	// word - search for lines containing the word
	if (words.size() == 1)
	{
		return std::shared_ptr<QueryBase>(new WordQuery(s));
	}

	//cout << "Unrecognized search";
	//return std::shared_ptr<QueryBase>(new WordQuery(""));
	return std::shared_ptr<QueryBase>(new WordQuery("Unrecognized search"));
}
//////////////////////////////////////////////////////////////////////////////
QueryResult NotQuery::eval(const TextQuery &text) const
{
	QueryResult result = text.query(query_word);
	auto ret_lines = std::make_shared<std::set<line_no>>();
	auto beg = result.begin(), end = result.end();
	auto sz = result.get_file()->size();

	for (size_t n = 0; n != sz; ++n)
	{
		if (beg == end || *beg != n)
			ret_lines->insert(n);
		else if (beg != end)
			++beg;
	}
	return QueryResult(rep(), ret_lines, result.get_file());
}

QueryResult AndQuery::eval(const TextQuery& text) const
{
	QueryResult left_result = text.query(left_query);
	QueryResult right_result = text.query(right_query);

	auto ret_lines = std::make_shared<std::set<line_no>>();
	std::set_intersection(left_result.begin(), left_result.end(),
		right_result.begin(), right_result.end(),
		std::inserter(*ret_lines, ret_lines->begin()));

	return QueryResult(rep(), ret_lines, left_result.get_file());
}

QueryResult OrQuery::eval(const TextQuery &text) const
{
	QueryResult left_result = text.query(left_query);
	QueryResult right_result = text.query(right_query);

	auto ret_lines =
		std::make_shared<std::set<line_no>>(left_result.begin(), left_result.end());

	ret_lines->insert(right_result.begin(), right_result.end());

	return QueryResult(rep(), ret_lines, left_result.get_file());
}
/////////////////////////////////////////////////////////
QueryResult NQuery::eval(const TextQuery &text) const
{
	QueryResult AND_res = AndQuery::eval(text);

	auto ret_lines = std::make_shared<std::set<line_no>>();

	shared_ptr<vector<string>> f = AND_res.get_file();

	regex exclamation_mark("[^\\w']+");
	smatch matches;

	// iterate only lines which contain both words
	for (auto it = AND_res.begin(); it != AND_res.end(); ++it)
	{
		vector<string> words;

		// separate each line into substrings
		istringstream line(f->at(*it));
		string word;

		// check each word in the line matches the regex
		while (line >> word)
		{
			if (regex_search(word, matches, exclamation_mark))
			{
				string newWord; // new word without the exclamation marks

				// regex iterator's begin and end
				auto words_begin =
					sregex_iterator(word.begin(), word.end(), exclamation_mark);

				auto words_end = sregex_iterator();

				// iterate all regex matches and remove them
				for (sregex_iterator it = words_begin; it != words_end; ++it)
				{
					newWord = regex_replace(word, exclamation_mark, " ");
				}

				// in case the regex match is in the middle of the string,
				// separate the string into more than a single string
				istringstream ln(newWord);
				string sub;

				while (ln >> sub)
				{
					words.push_back(sub);
				}
			}
			else // add words without the exclamation marks
			{
				words.push_back(word);
			}
		}

		int indl = 0; // index of 1st string
		int indr = 0; // index of 2nd string

		if (indl == 0 || indr == 0)
		{
			for (int i = 0; i < words.size(); i++)
			{
				if (words.at(i) == left_query)
				{
					indl = i;
				}

				if (words.at(i) == right_query)
				{
					indr = i;
				}
			}
		}

		int c = 0; // count separating strings
		int min = indl; // first string
		int max = indr; // second string

		if (min > max)
		{
			int tmp = min;
			min = max;
			max = tmp;
		}

		// count the number of words separating our sought words
		for (int i = min + 1; i < max; i++)
		{
			c++;
		}

		// no more than "dist" separating words
		if (c <= dist)
		{
			ret_lines->insert(*it);
		}

		words.clear();
	}

	return QueryResult(rep(), ret_lines, AND_res.get_file());
}
/////////////////////////////////////////////////////////