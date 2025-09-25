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
#include <cassert>
#include <sstream>
#include <chrono>

// 测试框架宏定义
#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::cerr << "Assertion failed: " << #expected << " == " << #actual \
                      << " (expected: " << (expected) << ", actual: " << (actual) << ")" \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Assertion failed: " << #condition \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            std::cerr << "Assertion failed: " << #condition \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define ASSERT_NEAR(expected, actual, tolerance) \
    do { \
        double diff = std::abs((expected) - (actual)); \
        if (diff > (tolerance)) { \
            std::cerr << "Assertion failed: " << #expected << " near " << #actual \
                      << " (expected: " << (expected) << ", actual: " << (actual) \
                      << ", diff: " << diff << ", tolerance: " << (tolerance) << ")" \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

// 测试用例基类
class TestCase {
public:
    virtual ~TestCase() = default;
    virtual bool run() = 0;
    virtual std::string getName() const = 0;
};

// 测试运行器
class TestRunner {
private:
    std::vector<std::unique_ptr<TestCase>> tests;
    int passed = 0;
    int failed = 0;

public:
    void addTest(std::unique_ptr<TestCase> test) {
        tests.push_back(std::move(test));
    }

    bool runAll() {
        std::cout << "Running " << tests.size() << " tests..." << std::endl;
        
        for (auto& test : tests) {
            std::cout << "Running " << test->getName() << "... ";
            try {
                if (test->run()) {
                    std::cout << "PASSED" << std::endl;
                    passed++;
                } else {
                    std::cout << "FAILED" << std::endl;
                    failed++;
                }
            } catch (const std::exception& e) {
                std::cout << "FAILED (exception: " << e.what() << ")" << std::endl;
                failed++;
            }
        }
        
        std::cout << "\nTest Results: " << passed << " passed, " << failed << " failed" << std::endl;
        return failed == 0;
    }
};

// 包含主程序的核心函数（复制过来用于测试）
bool is_cjk(uint32_t cp) {
    if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
        (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0x20000 && cp <= 0x2A6DF) ||
        (cp >= 0x2A700 && cp <= 0x2B73F) ||
        (cp >= 0x2B740 && cp <= 0x2B81F) ||
        (cp >= 0x2B820 && cp <= 0x2CEAF) ||
        (cp >= 0x2CEB0 && cp <= 0x2EBEF)) return true;
    return false;
}

bool is_keep_ascii(char c) {
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c >= '0' && c <= '9') return true;
    return false;
}

char to_lower_ascii(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    return c;
}

uint32_t utf8_next(const std::vector<unsigned char>& bytes, size_t& i) {
    if (i >= bytes.size()) return 0;
    unsigned char b0 = bytes[i];
    
    if (b0 < 0x80) {
        i++;
        return b0;
    }
    
    int seqlen = 0;
    uint32_t cp = 0;
    if ((b0 & 0xE0) == 0xC0) { seqlen = 2; cp = b0 & 0x1F; }
    else if ((b0 & 0xF0) == 0xE0) { seqlen = 3; cp = b0 & 0x0F; }
    else if ((b0 & 0xF8) == 0xF0) { seqlen = 4; cp = b0 & 0x07; }
    else { i++; return 0; }
    
    if (i + seqlen > bytes.size()) { i = bytes.size(); return 0; }
    
    for (int k = 1; k < seqlen; ++k) {
        unsigned char bx = bytes[i + k];
        if ((bx & 0xC0) != 0x80) { i++; return 0; }
        cp = (cp << 6) | (uint32_t)(bx & 0x3F);
    }
    
    if (seqlen == 2 && cp < 0x80) { i++; return 0; }
    if (seqlen == 3 && cp < 0x800) { i++; return 0; }
    if (seqlen == 4 && cp < 0x10000) { i++; return 0; }
    
    if (cp >= 0xD800 && cp <= 0xDFFF) { i++; return 0; }
    
    i += seqlen;
    return cp;
}

std::vector<uint32_t> normalize_to_codepoints(const std::vector<unsigned char>& bytes) {
    std::vector<uint32_t> codepoints;
    size_t i = 0;
    
    while (i < bytes.size()) {
        uint32_t cp = utf8_next(bytes, i);
        if (cp == 0) continue;
        
        if (cp < 128) {
            if (is_keep_ascii(static_cast<char>(cp))) {
                cp = to_lower_ascii(static_cast<char>(cp));
                codepoints.push_back(cp);
            }
        } else {
            if (is_cjk(cp)) {
                codepoints.push_back(cp);
            }
        }
    }
    
    return codepoints;
}

uint64_t fnv1a64_hash_kgram(const std::vector<uint32_t>& codepoints, size_t start, size_t k) {
    const uint64_t FNV_OFFSET = 1469598103934665603ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;
    uint64_t h = FNV_OFFSET;
    
    for (size_t i = 0; i < k; ++i) {
        uint32_t cp = codepoints[start + i];
        for (int b = 0; b < 4; ++b) {
            unsigned char x = static_cast<unsigned char>((cp >> (b * 8)) & 0xFFu);
            h ^= static_cast<uint64_t>(x);
            h *= FNV_PRIME;
        }
    }
    return h;
}

std::unordered_set<uint64_t> build_kgram_set(const std::vector<uint32_t>& codepoints, size_t k) {
    std::unordered_set<uint64_t> hash_set;
    if (k == 0 || codepoints.size() < k) return hash_set;
    
    size_t num = codepoints.size() - k + 1;
    
    for (size_t i = 0; i < num; ++i) {
        uint64_t hash = fnv1a64_hash_kgram(codepoints, i, k);
        hash_set.insert(hash);
    }
    
    return hash_set;
}

double jaccard_similarity(const std::unordered_set<uint64_t>& set1, const std::unordered_set<uint64_t>& set2) {
    if (set1.empty() && set2.empty()) return 0.0;
    
    size_t intersection = 0;
    for (const auto& hash : set1) {
        if (set2.find(hash) != set2.end()) {
            intersection++;
        }
    }
    
    size_t union_size = set1.size() + set2.size() - intersection;
    if (union_size == 0) return 0.0;
    
    return static_cast<double>(intersection) / static_cast<double>(union_size);
}

// 测试用例1：CJK字符识别
class TestCJKRecognition : public TestCase {
public:
    std::string getName() const override { return "CJK字符识别测试"; }
    
    bool run() override {
        // 测试中文字符
        ASSERT_TRUE(is_cjk(0x4E00)); // 一
        ASSERT_TRUE(is_cjk(0x9FFF)); // 最后一个CJK字符
        ASSERT_TRUE(is_cjk(0x6C49)); // 汉
        ASSERT_TRUE(is_cjk(0x5B57)); // 字
        
        // 测试非CJK字符
        ASSERT_FALSE(is_cjk(0x0041)); // A
        ASSERT_FALSE(is_cjk(0x0061)); // a
        ASSERT_FALSE(is_cjk(0x0030)); // 0
        ASSERT_FALSE(is_cjk(0x0020)); // 空格
        
        return true;
    }
};

// 测试用例2：ASCII字符处理
class TestASCIIProcessing : public TestCase {
public:
    std::string getName() const override { return "ASCII字符处理测试"; }
    
    bool run() override {
        // 测试保留字符
        ASSERT_TRUE(is_keep_ascii('A'));
        ASSERT_TRUE(is_keep_ascii('z'));
        ASSERT_TRUE(is_keep_ascii('0'));
        ASSERT_TRUE(is_keep_ascii('9'));
        
        // 测试不保留字符
        ASSERT_FALSE(is_keep_ascii(' '));
        ASSERT_FALSE(is_keep_ascii('.'));
        ASSERT_FALSE(is_keep_ascii(','));
        ASSERT_FALSE(is_keep_ascii('!'));
        
        // 测试大小写转换
        ASSERT_EQ('a', to_lower_ascii('A'));
        ASSERT_EQ('z', to_lower_ascii('Z'));
        ASSERT_EQ('a', to_lower_ascii('a')); // 已经小写
        ASSERT_EQ('0', to_lower_ascii('0')); // 数字不变
        
        return true;
    }
};

// 测试用例3：UTF-8解码
class TestUTF8Decoding : public TestCase {
public:
    std::string getName() const override { return "UTF-8解码测试"; }
    
    bool run() override {
        // 测试ASCII字符
        std::vector<unsigned char> ascii_bytes = {'H', 'e', 'l', 'l', 'o'};
        size_t i = 0;
        ASSERT_EQ('H', utf8_next(ascii_bytes, i));
        ASSERT_EQ('e', utf8_next(ascii_bytes, i));
        ASSERT_EQ('l', utf8_next(ascii_bytes, i));
        ASSERT_EQ('l', utf8_next(ascii_bytes, i));
        ASSERT_EQ('o', utf8_next(ascii_bytes, i));
        ASSERT_EQ(0, utf8_next(ascii_bytes, i)); // 超出范围
        
        // 测试中文字符 "你好"
        std::vector<unsigned char> chinese_bytes = {0xE4, 0xBD, 0xA0, 0xE5, 0xA5, 0xBD};
        i = 0;
        uint32_t cp1 = utf8_next(chinese_bytes, i);
        uint32_t cp2 = utf8_next(chinese_bytes, i);
        ASSERT_TRUE(cp1 > 0); // 应该成功解码
        ASSERT_TRUE(cp2 > 0); // 应该成功解码
        ASSERT_EQ(0, utf8_next(chinese_bytes, i)); // 超出范围
        
        return true;
    }
};

// 测试用例4：文本归一化
class TestTextNormalization : public TestCase {
public:
    std::string getName() const override { return "文本归一化测试"; }
    
    bool run() override {
        // 测试ASCII文本归一化
        std::string ascii_text = "Hello World! 123";
        std::vector<unsigned char> ascii_bytes(ascii_text.begin(), ascii_text.end());
        std::vector<uint32_t> codepoints = normalize_to_codepoints(ascii_bytes);
        
        // 应该只保留字母和数字，转换为小写
        std::vector<uint32_t> expected = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', '1', '2', '3'};
        ASSERT_EQ(expected.size(), codepoints.size());
        for (size_t i = 0; i < expected.size(); ++i) {
            ASSERT_EQ(expected[i], codepoints[i]);
        }
        
        return true;
    }
};

// 测试用例5：哈希函数
class TestHashFunction : public TestCase {
public:
    std::string getName() const override { return "哈希函数测试"; }
    
    bool run() override {
        std::vector<uint32_t> codepoints = {'h', 'e', 'l', 'l', 'o'};
        
        // 测试相同输入产生相同哈希
        uint64_t hash1 = fnv1a64_hash_kgram(codepoints, 0, 3);
        uint64_t hash2 = fnv1a64_hash_kgram(codepoints, 0, 3);
        ASSERT_EQ(hash1, hash2);
        
        // 测试不同输入产生不同哈希
        uint64_t hash3 = fnv1a64_hash_kgram(codepoints, 1, 3);
        ASSERT_TRUE(hash1 != hash3);
        
        return true;
    }
};

// 测试用例6：k-gram集合构建
class TestKGramSet : public TestCase {
public:
    std::string getName() const override { return "k-gram集合构建测试"; }
    
    bool run() override {
        std::vector<uint32_t> codepoints = {'a', 'b', 'c', 'd', 'e'};
        
        // 测试3-gram
        std::unordered_set<uint64_t> kgram_set = build_kgram_set(codepoints, 3);
        ASSERT_EQ(3, kgram_set.size()); // 应该有3个3-gram: abc, bcd, cde
        
        // 测试空输入
        std::vector<uint32_t> empty_codepoints;
        std::unordered_set<uint64_t> empty_set = build_kgram_set(empty_codepoints, 3);
        ASSERT_EQ(0, empty_set.size());
        
        // 测试k大于输入长度
        std::unordered_set<uint64_t> too_large_set = build_kgram_set(codepoints, 10);
        ASSERT_EQ(0, too_large_set.size());
        
        return true;
    }
};

// 测试用例7：Jaccard相似度计算
class TestJaccardSimilarity : public TestCase {
public:
    std::string getName() const override { return "Jaccard相似度计算测试"; }
    
    bool run() override {
        // 测试完全相同的集合
        std::unordered_set<uint64_t> set1 = {1, 2, 3, 4, 5};
        std::unordered_set<uint64_t> set2 = {1, 2, 3, 4, 5};
        double sim1 = jaccard_similarity(set1, set2);
        ASSERT_NEAR(1.0, sim1, 0.001);
        
        // 测试完全不同的集合
        std::unordered_set<uint64_t> set3 = {1, 2, 3};
        std::unordered_set<uint64_t> set4 = {4, 5, 6};
        double sim2 = jaccard_similarity(set3, set4);
        ASSERT_NEAR(0.0, sim2, 0.001);
        
        // 测试部分重叠的集合
        std::unordered_set<uint64_t> set5 = {1, 2, 3, 4};
        std::unordered_set<uint64_t> set6 = {3, 4, 5, 6};
        double sim3 = jaccard_similarity(set5, set6);
        ASSERT_NEAR(0.333, sim3, 0.01); // 交集2个，并集6个，相似度2/6=0.333
        
        // 测试空集合
        std::unordered_set<uint64_t> empty_set;
        double sim4 = jaccard_similarity(empty_set, empty_set);
        ASSERT_NEAR(0.0, sim4, 0.001);
        
        return true;
    }
};

// 测试用例8：端到端测试
class TestEndToEnd : public TestCase {
public:
    std::string getName() const override { return "端到端测试"; }
    
    bool run() override {
        // 创建测试文件
        std::string orig_text = "Hello World! 你好世界！";
        std::string plag_text = "hello world 你好世界";
        
        // 转换为字节向量
        std::vector<unsigned char> orig_bytes(orig_text.begin(), orig_text.end());
        std::vector<unsigned char> plag_bytes(plag_text.begin(), plag_text.end());
        
        // 归一化
        std::vector<uint32_t> orig_codepoints = normalize_to_codepoints(orig_bytes);
        std::vector<uint32_t> plag_codepoints = normalize_to_codepoints(plag_bytes);
        
        // 构建k-gram集合
        std::unordered_set<uint64_t> orig_set = build_kgram_set(orig_codepoints, 3);
        std::unordered_set<uint64_t> plag_set = build_kgram_set(plag_codepoints, 3);
        
        // 计算相似度
        double similarity = jaccard_similarity(orig_set, plag_set);
        
        // 相似度应该在合理范围内
        ASSERT_TRUE(similarity >= 0.0);
        ASSERT_TRUE(similarity <= 1.0);
        
        return true;
    }
};

// 测试用例9：边界条件测试
class TestBoundaryConditions : public TestCase {
public:
    std::string getName() const override { return "边界条件测试"; }
    
    bool run() override {
        // 测试空文件
        std::vector<unsigned char> empty_bytes;
        std::vector<uint32_t> empty_codepoints = normalize_to_codepoints(empty_bytes);
        ASSERT_EQ(0, empty_codepoints.size());
        
        // 测试只有标点符号
        std::string punct_text = "!@#$%^&*()";
        std::vector<unsigned char> punct_bytes(punct_text.begin(), punct_text.end());
        std::vector<uint32_t> punct_codepoints = normalize_to_codepoints(punct_bytes);
        ASSERT_EQ(0, punct_codepoints.size());
        
        // 测试只有空格
        std::string space_text = "   \t\n  ";
        std::vector<unsigned char> space_bytes(space_text.begin(), space_text.end());
        std::vector<uint32_t> space_codepoints = normalize_to_codepoints(space_bytes);
        ASSERT_EQ(0, space_codepoints.size());
        
        return true;
    }
};

// 测试用例10：性能测试
class TestPerformance : public TestCase {
public:
    std::string getName() const override { return "性能测试"; }
    
    bool run() override {
        // 创建较大的测试数据
        std::string large_text;
        for (int i = 0; i < 1000; ++i) {
            large_text += "Hello World! 你好世界！";
        }
        
        std::vector<unsigned char> large_bytes(large_text.begin(), large_text.end());
        
        // 测试归一化性能
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<uint32_t> codepoints = normalize_to_codepoints(large_bytes);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // 确保处理时间合理（小于1秒）
        ASSERT_TRUE(duration.count() < 1000);
        
        // 确保结果正确
        ASSERT_TRUE(codepoints.size() > 0);
        
        return true;
    }
};

int main() {
    TestRunner runner;
    
    // 添加所有测试用例
    runner.addTest(std::make_unique<TestCJKRecognition>());
    runner.addTest(std::make_unique<TestASCIIProcessing>());
    runner.addTest(std::make_unique<TestUTF8Decoding>());
    runner.addTest(std::make_unique<TestTextNormalization>());
    runner.addTest(std::make_unique<TestHashFunction>());
    runner.addTest(std::make_unique<TestKGramSet>());
    runner.addTest(std::make_unique<TestJaccardSimilarity>());
    runner.addTest(std::make_unique<TestEndToEnd>());
    runner.addTest(std::make_unique<TestBoundaryConditions>());
    runner.addTest(std::make_unique<TestPerformance>());
    
    // 运行所有测试
    bool success = runner.runAll();
    
    return success ? 0 : 1;
}
