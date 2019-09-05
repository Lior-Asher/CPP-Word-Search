#include "TextQuery.h"
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <regex>
#include <iterator>
using namespace std;

// read the input file and build the map of lines to line numbers
TextQuery::TextQuery(ifstream &is) : file(new vector<string>)
{
	string text;
	regex words_regex("[\\w']+");
	regex exclamation_mark("[^\\w']+");
	smatch matches;

	while (getline(is, text)) {       // for each line in the file
		file->push_back(text);        // remember this line of text
		int n = file->size() - 1;     // the current line number
//////////////////////////////////////////////////////////////////////////
		istringstream line(text);     // separate the line into words
		string word;

		while (line >> word) {        // for each word in that line

			// if a word contains exclamation marks remove them
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
					// if word isn't already in wm, subscripting adds a new entry
					auto &lines = wm[sub]; // lines is a shared_ptr 
					if (!lines) // that pointer is null the first time we see word
						lines.reset(new set<line_no>); // allocate a new set
					lines->insert(n);      // insert this line number
				}
			}
			else // add words without the exclamation marks
			{
				auto &lines = wm[word]; // lines is a shared_ptr 
				if (!lines) // that pointer is null the first time we see word
					lines.reset(new set<line_no>); // allocate a new set
				lines->insert(n);      // insert this line number
			}
		}
//////////////////////////////////////////////////////////////////////////
	}
}

QueryResult TextQuery::query(const string &sought) const
{
	// we'll return a pointer to this set if we don't find sought
	static shared_ptr<set<line_no>> nodata(new set<line_no>);
	// we use find instead of subscript, to avoid adding words to wm
	auto loc = wm.find(sought);
	if (loc == wm.end())
		return QueryResult(sought, nodata, file);  // not found
	else
		return QueryResult(sought, loc->second, file);
}

// debugging routine
void TextQuery::display_map()
{
	auto iter = wm.cbegin(), iter_end = wm.cend();
	for (; iter != iter_end; ++iter) {
		cout << iter->first << ": {";
		// fetch location vector as a const reference to avoid copying it
		auto text_locs = iter->second;
		auto loc_iter = text_locs->cbegin(),
			loc_iter_end = text_locs->cend();
		// print all line numbers for this word
		while (loc_iter != loc_iter_end)
		{
			cout << *loc_iter + 1;
			if (++loc_iter != loc_iter_end)
				cout << ", ";

		}
		cout << "}\n";  // end list of output this word
	}
	cout << endl;  // finished printing entire map
}

std::ostream &print(std::ostream &os, const QueryResult &qr)
{
	if (qr.sought == "Unrecognized search")
	{
		os << "Unrecognized search";
		return os;
	}

	os << "\"" << qr.sought << "\"" << " occours " <<
		qr.lines->size() << " times:" << std::endl;

	for (auto num : *qr.lines)
	{
		os << "\t(line " << num + 1 << ") "
			<< *(qr.file->begin() + num) << std::endl;
	}

	return os;
}
