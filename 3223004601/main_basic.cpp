#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
#include <cstdint>
#include <iomanip>
#include <stdexcept>

// --- minimal core from main.cpp for Basic commit ---

bool is_cjk(uint32_t cp)
{
  if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
      (cp >= 0x3400 && cp <= 0x4DBF) ||
      (cp >= 0xF900 && cp <= 0xFAFF) ||
      (cp >= 0x20000 && cp <= 0x2A6DF) ||
      (cp >= 0x2A700 && cp <= 0x2B73F) ||
      (cp >= 0x2B740 && cp <= 0x2B81F) ||
      (cp >= 0x2B820 && cp <= 0x2CEAF) ||
      (cp >= 0x2CEB0 && cp <= 0x2EBEF))
    return true;
  return false;
}

bool is_keep_ascii(char c)
{
  if (c >= 'A' && c <= 'Z')
    return true;
  if (c >= 'a' && c <= 'z')
    return true;
  if (c >= '0' && c <= '9')
    return true;
  return false;
}

char to_lower_ascii(char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';
  return c;
}

uint32_t utf8_next(const std::vector<unsigned char> &bytes, size_t &i)
{
  if (i >= bytes.size())
    return 0;
  unsigned char b0 = bytes[i];
  if (b0 < 0x80)
  {
    i++;
    return b0;
  }
  int seqlen = 0;
  uint32_t cp = 0;
  if ((b0 & 0xE0) == 0xC0)
  {
    seqlen = 2;
    cp = b0 & 0x1F;
  }
  else if ((b0 & 0xF0) == 0xE0)
  {
    seqlen = 3;
    cp = b0 & 0x0F;
  }
  else if ((b0 & 0xF8) == 0xF0)
  {
    seqlen = 4;
    cp = b0 & 0x07;
  }
  else
  {
    i++;
    return 0;
  }
  if (i + seqlen > bytes.size())
  {
    i = bytes.size();
    return 0;
  }
  for (int k = 1; k < seqlen; ++k)
  {
    unsigned char bx = bytes[i + k];
    if ((bx & 0xC0) != 0x80)
    {
      i++;
      return 0;
    }
    cp = (cp << 6) | (uint32_t)(bx & 0x3F);
  }
  if (seqlen == 2 && cp < 0x80)
  {
    i++;
    return 0;
  }
  if (seqlen == 3 && cp < 0x800)
  {
    i++;
    return 0;
  }
  if (seqlen == 4 && cp < 0x10000)
  {
    i++;
    return 0;
  }
  if (cp >= 0xD800 && cp <= 0xDFFF)
  {
    i++;
    return 0;
  }
  i += seqlen;
  return cp;
}

std::vector<unsigned char> read_file_to_bytes(const std::string &path)
{
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Failed to open file: " + path);
  file.seekg(0, std::ios::end);
  auto size = static_cast<size_t>(file.tellg());
  if (size == 0)
    return {};
  file.seekg(0, std::ios::beg);
  std::vector<unsigned char> bytes(size);
  file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size));
  return bytes;
}

std::vector<uint32_t> normalize_to_codepoints(const std::vector<unsigned char> &bytes)
{
  std::vector<uint32_t> codepoints;
  size_t i = 0;
  while (i < bytes.size())
  {
    uint32_t cp = utf8_next(bytes, i);
    if (cp == 0)
      continue;
    if (cp < 128)
    {
      if (is_keep_ascii(static_cast<char>(cp)))
      {
        cp = to_lower_ascii(static_cast<char>(cp));
        codepoints.push_back(cp);
      }
    }
    else
    {
      if (is_cjk(cp))
        codepoints.push_back(cp);
    }
  }
  return codepoints;
}

uint64_t fnv1a64_hash_kgram(const std::vector<uint32_t> &codepoints, size_t start, size_t k)
{
  const uint64_t FNV_OFFSET = 1469598103934665603ULL;
  const uint64_t FNV_PRIME = 1099511628211ULL;
  uint64_t h = FNV_OFFSET;
  for (size_t i = 0; i < k; ++i)
  {
    uint32_t cp = codepoints[start + i];
    for (int b = 0; b < 4; ++b)
    {
      unsigned char x = static_cast<unsigned char>((cp >> (b * 8)) & 0xFFu);
      h ^= static_cast<uint64_t>(x);
      h *= FNV_PRIME;
    }
  }
  return h;
}

std::unordered_set<uint64_t> build_kgram_set(const std::vector<uint32_t> &codepoints, size_t k)
{
  std::unordered_set<uint64_t> hash_set;
  if (k == 0 || codepoints.size() < k)
    return hash_set;
  for (size_t i = 0; i + k <= codepoints.size(); ++i)
  {
    uint64_t h = fnv1a64_hash_kgram(codepoints, i, k);
    hash_set.insert(h);
  }
  return hash_set;
}

double jaccard_similarity(const std::unordered_set<uint64_t> &a, const std::unordered_set<uint64_t> &b)
{
  if (a.empty() && b.empty())
    return 0.0;
  size_t inter = 0;
  for (auto &x : a)
    if (b.find(x) != b.end())
      inter++;
  size_t uni = a.size() + b.size() - inter;
  if (uni == 0)
    return 0.0;
  return static_cast<double>(inter) / static_cast<double>(uni);
}

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    std::cerr << "Usage: " << argv[0] << " <orig> <plag> <out>\n";
    return 1;
  }
  try
  {
    auto b1 = read_file_to_bytes(argv[1]);
    auto b2 = read_file_to_bytes(argv[2]);
    auto cp1 = normalize_to_codepoints(b1);
    auto cp2 = normalize_to_codepoints(b2);
    const size_t K = 3;
    auto s1 = build_kgram_set(cp1, K);
    auto s2 = build_kgram_set(cp2, K);
    double sim = jaccard_similarity(s1, s2);
    std::ofstream fout(argv[3]);
    fout << std::fixed << std::setprecision(2) << sim;
    return 0;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
