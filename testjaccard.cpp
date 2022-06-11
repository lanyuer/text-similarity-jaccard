#include <wchar.h>
#include <regex>
#include <string>
#include <iostream>
#include <codecvt>
#include <iomanip>
#include <vector>
#include <unordered_set>

#include "cppjieba/Jieba.hpp"

const char* const DICT_PATH = "./dict/jieba.dict.utf8";
const char* const HMM_PATH = "./dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "./dict/user.dict.utf8";
const char* const IDF_PATH = "./dict/idf.utf8";
const char* const STOP_WORD_PATH = "./dict/bd_stopwords.txt";

using namespace std;


std::wstring s2ws(const std::string& utf8Str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(utf8Str);
}

std::string ws2s(const std::wstring& utf16Str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(utf16Str);
}

std::string remove_punc(const std::string& str)
{
    const std::wstring punc = L"＂＃＄％＆\\\\＇（）＊＋，－／：；＜＝＞＠［＼］＾＿｀｛｜｝～｟｠｢｣､　、〃〈〉《》「」『』【】〔〕〖〗〘〙〚〛〜〝〞〟〰〾〿–—‘’‛“”„‟…‧﹏﹑﹔·！？｡。!#$%^&*()_+-= \t\n|\';\":/.,?\\><~";
    wregex re(L"["+punc+L"]");

    std::wstring wstr = s2ws(str);
    wstring rstr = regex_replace(wstr, re, L"");
    return ws2s(rstr);
}

class Jaccrad_similar
{
public:
  Jaccrad_similar() : _jieba(nullptr) {};
  ~Jaccrad_similar() {
    if (_jieba != nullptr) {
      delete _jieba;
      _jieba = nullptr;
    }
  }
  int init(const char* dict, const char* hmm, const char* userdict, const char* idf, const char* stopwords);

  int get_grams(const std::string& str, std::vector<std::string>& grams);
  float calc_coeff(const std::string& strA, const std::string& strB);

protected:
  int load_stopwords(const char* file);

private:
  unordered_set<string> _stop_words;
  cppjieba::Jieba *_jieba;
};

int Jaccrad_similar::init(const char* dict, const char* hmm, const char* userdict, const char* idf, const char* stopwords)
{
  _jieba = new cppjieba::Jieba(dict, hmm, userdict, idf, stopwords);
  load_stopwords(STOP_WORD_PATH);
  return 0;
}

int Jaccrad_similar::load_stopwords(const char* file)
{
    ifstream fs(file);
    std::copy(istream_iterator<std::string>(fs), istream_iterator<std::string>(), inserter(_stop_words, _stop_words.end()));
    return 0;
}

int Jaccrad_similar::get_grams(const std::string& str, std::vector<std::string>& grams)
{
    string doc = remove_punc(str);
    
    //cout << "[demo] Cut With HMM" << endl;
    _jieba->Cut(doc, grams, true);
    //cout << limonp::Join(grams.begin(), grams.end(), "/") << endl;

    for (auto i = grams.begin(); i != grams.end(); ) {
        auto it = _stop_words.find(*i);
        if (_stop_words.find(*i) != _stop_words.end())
        {
            i = grams.erase(i);
        }
        else
            i++;
    }
    // cout << limonp::Join(grams.begin(), grams.end(), "/") << endl;
    return 0;
}

float Jaccrad_similar::calc_coeff(const std::string& strA, const std::string& strB)
{
  if (strA.empty() && strB.empty()) return 1.0f;
  if (strA.empty() || strB.empty()) return 0.0f;

  vector<string> gramsA, gramsB;
  get_grams(strA, gramsA);
  get_grams(strB, gramsB);

  unordered_set<string> words_union, words_intersection;
  for(auto i: gramsA) words_union.insert(i);
  for(auto i: gramsB) words_union.insert(i);
  
  unordered_set<string> set{gramsA.cbegin(), gramsA.cend()};
  for (auto n: gramsB) {
    if (set.erase(n) > 0) // if n exists in set, then 1 is returned and n is erased; otherwise, 0.
      words_intersection.insert(n);
  }

  int len_i = words_intersection.size();
  // len_u = gramsA.size() + gramsB.size();
  int len_u = words_union.size();
  return float(len_i)/len_u;
}


int main(int argc, char* argv[])
{
    vector<string> documents = {
        "中  一遍又一遍地  “真”、“好”和“吃”  上摔下来，  爬上结满很多红果子  是伤，还不  就在这时  的>树。可是，他们都  大会爬树",
        "“真”、“好”和“吃” 爬上结满很多红果子 的树。可是，他们都 不太会爬树，, ",
        " “真”、“好”和“吃”  爬上结满很多红果子  的树。可是，他们都  不太会爬树， , ",
        "一遍又一遍地 上摔下来，满 真”、“好”和“吃” 是伤，还不愿 飞上结满很多红果子 就在这时· 的树。可>是，他们都 不太会爬树，,",
        "一遍又一遍地  上摔下来，满  真”、“好”和“吃”  是伤，还不愿  飞上结满很多红果子  就在这时·  的>树。可是，他们都  不太会爬树， , ",
        "一遍又一遍地 “好”和“吃” 上摔下来，满 满很多红果子 是伤，还不愿 可是，他们都 就在这时…· 爬树",
        "一遍又一遍地  “好”和“吃”  上摔下来，满  满很多红果子  是伤，还不愿  可是，他们都  就在这时…·  爬树， , "      
    };

    Jaccrad_similar sim;
    sim.init(DICT_PATH, HMM_PATH, 
        USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);

    for (int i=0; i<documents.size(); ++i) {
      for (int x=0; x<i; ++x) cout << "++++ ";
      for (int j=i; j<documents.size(); ++j) {
        float coeff = sim.calc_coeff(documents.at(i), documents.at(j));
        cout << setiosflags(ios::fixed) << setprecision(2) << coeff << " ";
      }
      cout << endl;
    }

    
    return 0;
}
