#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <stdexcept>

// Visual Studio 2017 兼容性宏
#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4996) // 禁用不安全函数警告
    #define _CRT_SECURE_NO_WARNINGS
#endif

// 判断是否为中日韩统一表意文字（CJK字符）
bool is_cjk(uint32_t cp) {
    // 检查Unicode码点是否在CJK字符范围内
    if ((cp >= 0x4E00 && cp <= 0x9FFF) ||      // CJK统一表意文字
        (cp >= 0x3400 && cp <= 0x4DBF) ||      // CJK扩展A
        (cp >= 0xF900 && cp <= 0xFAFF) ||      // CJK兼容表意文字
        (cp >= 0x20000 && cp <= 0x2A6DF) ||    // CJK扩展B
        (cp >= 0x2A700 && cp <= 0x2B73F) ||    // CJK扩展C
        (cp >= 0x2B740 && cp <= 0x2B81F) ||    // CJK扩展D
        (cp >= 0x2B820 && cp <= 0x2CEAF) ||    // CJK扩展E
        (cp >= 0x2CEB0 && cp <= 0x2EBEF)) return true;  // CJK扩展F
    return false;
}

// 判断ASCII字符是否应该保留（字母和数字）
bool is_keep_ascii(char c) {
    if (c >= 'A' && c <= 'Z') return true;  // 大写字母
    if (c >= 'a' && c <= 'z') return true;  // 小写字母
    if (c >= '0' && c <= '9') return true;  // 数字
    return false;  // 其他字符（标点、空格等）不保留
}

// 将ASCII大写字母转换为小写
char to_lower_ascii(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    return c;  // 非大写字母直接返回
}

// UTF-8解码器：从字节流中读取下一个Unicode码点，并推进索引
// 遇到无效字节时跳过（返回0表示跳过）
uint32_t utf8_next(const std::vector<unsigned char>& bytes, size_t& i) {
    if (i >= bytes.size()) return 0;  // 超出范围
    unsigned char b0 = bytes[i];
    
    // 单字节字符（ASCII）
    if (b0 < 0x80) {
        i++;
        return b0;
    }
    
    // 确定多字节序列长度
    int seqlen = 0;
    uint32_t cp = 0;
    if ((b0 & 0xE0) == 0xC0) { seqlen = 2; cp = b0 & 0x1F; }      // 2字节序列
    else if ((b0 & 0xF0) == 0xE0) { seqlen = 3; cp = b0 & 0x0F; }  // 3字节序列
    else if ((b0 & 0xF8) == 0xF0) { seqlen = 4; cp = b0 & 0x07; }  // 4字节序列
    else { i++; return 0; }  // 无效的首字节
    
    // 检查是否有足够的字节
    if (i + seqlen > bytes.size()) { i = bytes.size(); return 0; }
    
    // 读取后续字节
    for (int k = 1; k < seqlen; ++k) {
        unsigned char bx = bytes[i + k];
        if ((bx & 0xC0) != 0x80) { i++; return 0; }  // 无效的后续字节
        cp = (cp << 6) | (uint32_t)(bx & 0x3F);  // 组合码点
    }
    
    // 检查过短编码（overlong encoding）
    if (seqlen == 2 && cp < 0x80) { i++; return 0; }
    if (seqlen == 3 && cp < 0x800) { i++; return 0; }
    if (seqlen == 4 && cp < 0x10000) { i++; return 0; }
    
    // UTF-16代理对在UTF-8中无效
    if (cp >= 0xD800 && cp <= 0xDFFF) { i++; return 0; }
    
    i += seqlen;
    return cp;
}

// 将文件内容读取到字节向量
std::vector<unsigned char> read_file_to_bytes(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    
    // 获取文件大小
    file.seekg(0, std::ios::end);
    auto size = static_cast<size_t>(file.tellg());
    if (size == 0) {
        return {}; // 空文件
    }
    file.seekg(0, std::ios::beg);
    
    // 读取文件内容
    std::vector<unsigned char> bytes(size);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
    
    if (file.fail() && !file.eof()) {
        throw std::runtime_error("Failed to read file: " + path);
    }
    
    return bytes;
}

// 将UTF-8字节流转换为归一化的码点序列
// 只保留字母、数字（转小写）和CJK字符，过滤标点符号和空白
std::vector<uint32_t> normalize_to_codepoints(const std::vector<unsigned char>& bytes) {
    std::vector<uint32_t> codepoints;
    size_t i = 0;
    
    while (i < bytes.size()) {
        uint32_t cp = utf8_next(bytes, i);
        if (cp == 0) continue; // 跳过无效字符
        
        if (cp < 128) {  // ASCII字符
            if (is_keep_ascii(static_cast<char>(cp))) {  // 只保留字母和数字
                cp = to_lower_ascii(static_cast<char>(cp));  // 转换为小写
                codepoints.push_back(cp);
            }
            // 其他ASCII字符（标点、空格等）直接丢弃
        } else {  // 非ASCII字符
            if (is_cjk(cp)) {  // 只保留CJK字符
                codepoints.push_back(cp);
            }
            // 其他非ASCII字符（如拉丁扩展字符）直接丢弃
        }
    }
    
    return codepoints;
}

// 使用FNV-1a算法计算k-gram的64位哈希值
uint64_t fnv1a64_hash_kgram(const std::vector<uint32_t>& codepoints, size_t start, size_t k) {
    const uint64_t FNV_OFFSET = 1469598103934665603ULL;  // FNV-1a偏移量
    const uint64_t FNV_PRIME = 1099511628211ULL;          // FNV-1a质数
    uint64_t h = FNV_OFFSET;
    
    for (size_t i = 0; i < k; ++i) {
        uint32_t cp = codepoints[start + i];
        // 将码点的4个字节分别混合到哈希中
        for (int b = 0; b < 4; ++b) {
            unsigned char x = static_cast<unsigned char>((cp >> (b * 8)) & 0xFFu);
            h ^= static_cast<uint64_t>(x);  // 异或操作
            h *= FNV_PRIME;    // 乘法操作
        }
    }
    return h;
}

// 构建k-gram哈希集合，生成所有k-gram的哈希值并去重
std::unordered_set<uint64_t> build_kgram_set(const std::vector<uint32_t>& codepoints, size_t k) {
    std::unordered_set<uint64_t> hash_set;
    if (k == 0 || codepoints.size() < k) return hash_set;  // 参数无效或文本太短
    
    size_t num = codepoints.size() - k + 1;  // k-gram的总数量
    
    // 计算每个k-gram的哈希值并插入到集合中（自动去重）
    for (size_t i = 0; i < num; ++i) {
        uint64_t hash = fnv1a64_hash_kgram(codepoints, i, k);
        hash_set.insert(hash);
    }
    
    return hash_set;
}

// 计算Jaccard相似度：交集大小 / 并集大小
double jaccard_similarity(const std::unordered_set<uint64_t>& set1, const std::unordered_set<uint64_t>& set2) {
    if (set1.empty() && set2.empty()) return 0.0;  // 两个空集合
    
    // 计算交集大小
    size_t intersection = 0;
    for (const auto& hash : set1) {
        if (set2.find(hash) != set2.end()) {
            intersection++;
        }
    }
    
    // 计算并集大小
    size_t union_size = set1.size() + set2.size() - intersection;
    if (union_size == 0) return 0.0;  // 避免除零
    
    return static_cast<double>(intersection) / static_cast<double>(union_size);  // Jaccard相似度
}

int main(int argc, char** argv) {
    try {
        // 检查命令行参数数量
        if (argc != 4) {
            std::cerr << "Usage: " << argv[0] << " <orig_file> <plagiarized_file> <answer_file>" << std::endl;
            return 1;
        }
        
        // 获取文件路径参数
        std::string path_orig = argv[1];   // 原文文件路径
        std::string path_plag = argv[2];   // 抄袭版文件路径
        std::string path_out = argv[3];    // 答案文件路径

        // 读取文件内容到字节向量
        std::vector<unsigned char> bytes1 = read_file_to_bytes(path_orig);
        std::vector<unsigned char> bytes2 = read_file_to_bytes(path_plag);

        // 将文件内容转换为归一化的码点序列
        std::vector<uint32_t> codepoints1 = normalize_to_codepoints(bytes1);  // 处理原文
        std::vector<uint32_t> codepoints2 = normalize_to_codepoints(bytes2);  // 处理抄袭版

        // 构建3-gram哈希集合（3-gram是文本相似度计算的常用方法）
        constexpr size_t K = 3;
        std::unordered_set<uint64_t> hash_set1 = build_kgram_set(codepoints1, K);  // 原文的k-gram集合
        std::unordered_set<uint64_t> hash_set2 = build_kgram_set(codepoints2, K);  // 抄袭版的k-gram集合

        // 计算Jaccard相似度
        double sim = jaccard_similarity(hash_set1, hash_set2);

        // 将结果写入答案文件
        std::ofstream fout(path_out, std::ios::binary);
        if (!fout.is_open()) {
            std::cerr << "Failed to open output: " << path_out << std::endl;
            return 1;
        }
        
        // 输出相似度，保留两位小数
        fout << std::fixed << std::setprecision(2) << sim;
        fout.close();

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}

#ifdef _MSC_VER
    #pragma warning(pop)
#endif
