#pragma once

#include <string>
#include <vector>
#include <map>

namespace PaperCrawler {

/**
 * @brief Mock数据库数据（临时方案）
 */
class MockDatabaseData {
public:
    static std::vector<std::map<std::string, std::string>> getPapers() {
        std::vector<std::map<std::string, std::string>> papers;

        std::map<std::string, std::string> paper1;
        paper1["id"] = "1";
        paper1["title"] = "Attention Is All You Need";
        paper1["authors"] = "Ashish Vaswani et al.";
        paper1["year"] = "2017";
        paper1["publication"] = "NeurIPS";
        paper1["citation_count"] = "50000";
        papers.push_back(paper1);

        std::map<std::string, std::string> paper2;
        paper2["id"] = "2";
        paper2["title"] = "BERT: Pre-training of Deep Bidirectional Transformers";
        paper2["authors"] = "Jacob Devlin et al.";
        paper2["year"] = "2018";
        paper2["publication"] = "NAACL";
        paper2["citation_count"] = "80000";
        papers.push_back(paper2);

        std::map<std::string, std::string> paper3;
        paper3["id"] = "3";
        paper3["title"] = "ResNet: Deep Residual Learning for Image Recognition";
        paper3["authors"] = "Kaiming He et al.";
        paper3["year"] = "2015";
        paper3["publication"] = "CVPR";
        paper3["citation_count"] = "120000";
        papers.push_back(paper3);

        std::map<std::string, std::string> paper4;
        paper4["id"] = "4";
        paper4["title"] = "GPT-4 Technical Report";
        paper4["authors"] = "OpenAI";
        paper4["year"] = "2023";
        paper4["publication"] = "arXiv";
        paper4["citation_count"] = "10000";
        papers.push_back(paper4);

        std::map<std::string, std::string> paper5;
        paper5["id"] = "5";
        paper5["title"] = "Deep Residual Learning for Image Recognition";
        paper5["authors"] = "Kaiming He et al.";
        paper5["year"] = "2016";
        paper5["publication"] = "CVPR";
        paper5["citation_count"] = "150000";
        papers.push_back(paper5);

        return papers;
    }

    static std::vector<std::map<std::string, std::string>> getJournals() {
        std::vector<std::map<std::string, std::string>> journals;

        std::map<std::string, std::string> journal1;
        journal1["name"] = "NeurIPS";
        journal1["tier"] = "Tier 1";
        journal1["impact_factor"] = "8.2";
        journals.push_back(journal1);

        std::map<std::string, std::string> journal2;
        journal2["name"] = "CVPR";
        journal2["tier"] = "Tier 1";
        journal2["impact_factor"] = "9.5";
        journals.push_back(journal2);

        std::map<std::string, std::string> journal3;
        journal3["name"] = "ICML";
        journal3["tier"] = "Tier 1";
        journal3["impact_factor"] = "8.7";
        journals.push_back(journal3);

        std::map<std::string, std::string> journal4;
        journal4["name"] = "NAACL";
        journal4["tier"] = "Tier 1";
        journal4["impact_factor"] = "6.5";
        journals.push_back(journal4);

        return journals;
    }

    static std::vector<std::map<std::string, std::string>> getAuthors() {
        std::vector<std::map<std::string, std::string>> authors;

        std::map<std::string, std::string> author1;
        author1["id"] = "1";
        author1["name"] = "Geoffrey Hinton";
        author1["email"] = "hinton@utoronto.ca";
        author1["affiliation"] = "University of Toronto";
        author1["h_index"] = "160";
        authors.push_back(author1);

        std::map<std::string, std::string> author2;
        author2["id"] = "2";
        author2["name"] = "Yann LeCun";
        author2["email"] = "lecun@nyu.edu";
        author2["affiliation"] = "New York University";
        author2["h_index"] = "140";
        authors.push_back(author2);

        std::map<std::string, std::string> author3;
        author3["id"] = "3";
        author3["name"] = "Yoshua Bengio";
        author3["email"] = "bengio@montreal.ca";
        author3["affiliation"] = "University of Montreal";
        author3["h_index"] = "150";
        authors.push_back(author3);

        return authors;
    }
};

} // namespace PaperCrawler
