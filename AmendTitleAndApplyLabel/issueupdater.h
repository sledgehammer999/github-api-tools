/* MIT License

Copyright (c) 2020 sledgehammer999 <hammered999@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct IssueAttributes;
class PostDownloader;

class IssueUpdater
{
public:
    // The passed arguments must outlive the class instance
    explicit IssueUpdater(PostDownloader &downloader,
                          const std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> &issues,
                          std::string &error);

    void run();
    std::string nextBatch();
    bool hasNextBatch();

private:
    void onFinishedPage();

    void gatherIssues(std::string_view response);
    std::string makeIssueAlias(const int counter, const IssueAttributes &attr);
    std::string makeLabelArray(const std::vector<std::string> &labelIDs);

    PostDownloader &m_downloader;
    const std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>> &m_issues;
    std::string &m_error;
    std::unordered_map<std::vector<int>::size_type, std::vector<IssueAttributes>>::const_iterator m_regexPos;
    int m_issuePos = 0;
    bool m_hasNextBatch;
};
