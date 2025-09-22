#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <cstdint>
#include <iomanip>

// 性能测试工具类
class PerformanceProfiler {
private:
    std::vector<std::pair<std::string, double>> timings;
    
public:
    void startTimer(const std::string& name) {
        // 这里可以添加更复杂的性能分析代码
        std::cout << "Starting timer for: " << name << std::endl;
    }
    
    void endTimer(const std::string& name, double duration) {
        timings.push_back({name, duration});
        std::cout << "Timer " << name << " completed in " << duration << " ms" << std::endl;
    }
    
    void printReport() {
        std::cout << "\n=== Performance Report ===" << std::endl;
        std::cout << std::left << std::setw(30) << "Operation" << std::setw(15) << "Time (ms)" << std::endl;
        std::cout << std::string(45, '-') << std::endl;
        
        for (const auto& timing : timings) {
            std::cout << std::left << std::setw(30) << timing.first 
                      << std::setw(15) << std::fixed << std::setprecision(2) << timing.second << std::endl;
        }
    }
};

// 包含核心算法（复制自主程序）
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
    codepoints.reserve(bytes.size()); // 预分配内存以提高性能
    
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
    hash_set.reserve(num); // 预分配内存以提高性能
    
    for (size_t i = 0; i < num; ++i) {
        uint64_t hash = fnv1a64_hash_kgram(codepoints, i, k);
        hash_set.insert(hash);
    }
    
    return hash_set;
}

double jaccard_similarity(const std::unordered_set<uint64_t>& set1, const std::unordered_set<uint64_t>& set2) {
    if (set1.empty() && set2.empty()) return 0.0;
    
    size_t intersection = 0;
    // 优化：遍历较小的集合
    if (set1.size() <= set2.size()) {
        for (const auto& hash : set1) {
            if (set2.find(hash) != set2.end()) {
                intersection++;
            }
        }
    } else {
        for (const auto& hash : set2) {
            if (set1.find(hash) != set1.end()) {
                intersection++;
            }
        }
    }
    
    size_t union_size = set1.size() + set2.size() - intersection;
    if (union_size == 0) return 0.0;
    
    return static_cast<double>(intersection) / static_cast<double>(union_size);
}

// 生成测试数据
std::vector<unsigned char> generateTestData(size_t size, bool include_chinese = true) {
    std::vector<unsigned char> data;
    data.reserve(size);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> ascii_dist(32, 126);
    std::uniform_int_distribution<> chinese_dist(0x4E00, 0x9FFF);
    
    for (size_t i = 0; i < size; ++i) {
        if (include_chinese && i % 10 == 0) {
            // 每10个字符插入一个中文字符
            uint32_t chinese_char = chinese_dist(gen);
            // 转换为UTF-8字节
            if (chinese_char < 0x800) {
                data.push_back(0xE0 | (chinese_char >> 12));
                data.push_back(0x80 | ((chinese_char >> 6) & 0x3F));
                data.push_back(0x80 | (chinese_char & 0x3F));
            }
        } else {
            data.push_back(static_cast<unsigned char>(ascii_dist(gen)));
        }
    }
    
    return data;
}

// 性能测试函数
void runPerformanceTests() {
    PerformanceProfiler profiler;
    
    std::cout << "=== Plagiarism Detector Performance Test ===" << std::endl;
    
    // 测试不同大小的数据
    std::vector<size_t> test_sizes = {1000, 10000, 100000, 1000000};
    
    for (size_t size : test_sizes) {
        std::cout << "\n--- Testing with " << size << " bytes ---" << std::endl;
        
        // 生成测试数据
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<unsigned char> data1 = generateTestData(size);
        std::vector<unsigned char> data2 = generateTestData(size);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        profiler.endTimer("Data Generation (" + std::to_string(size) + " bytes)", duration.count());
        
        // 测试文本归一化性能
        start = std::chrono::high_resolution_clock::now();
        std::vector<uint32_t> codepoints1 = normalize_to_codepoints(data1);
        std::vector<uint32_t> codepoints2 = normalize_to_codepoints(data2);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        profiler.endTimer("Text Normalization (" + std::to_string(size) + " bytes)", duration.count());
        
        // 测试k-gram构建性能
        start = std::chrono::high_resolution_clock::now();
        std::unordered_set<uint64_t> set1 = build_kgram_set(codepoints1, 3);
        std::unordered_set<uint64_t> set2 = build_kgram_set(codepoints2, 3);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        profiler.endTimer("K-gram Building (" + std::to_string(size) + " bytes)", duration.count());
        
        // 测试相似度计算性能
        start = std::chrono::high_resolution_clock::now();
        double similarity = jaccard_similarity(set1, set2);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        profiler.endTimer("Similarity Calculation (" + std::to_string(size) + " bytes)", duration.count());
        
        std::cout << "Similarity: " << std::fixed << std::setprecision(4) << similarity << std::endl;
    }
    
    // 内存使用分析
    std::cout << "\n--- Memory Usage Analysis ---" << std::endl;
    std::vector<unsigned char> large_data = generateTestData(1000000);
    std::vector<uint32_t> codepoints = normalize_to_codepoints(large_data);
    std::unordered_set<uint64_t> kgram_set = build_kgram_set(codepoints, 3);
    
    std::cout << "Original data size: " << large_data.size() << " bytes" << std::endl;
    std::cout << "Normalized codepoints: " << codepoints.size() << " * 4 = " 
              << codepoints.size() * 4 << " bytes" << std::endl;
    std::cout << "K-gram set size: " << kgram_set.size() << " * 8 = " 
              << kgram_set.size() * 8 << " bytes" << std::endl;
    std::cout << "Total memory usage: " 
              << large_data.size() + codepoints.size() * 4 + kgram_set.size() * 8 
              << " bytes" << std::endl;
    
    profiler.printReport();
}

// 基准测试
void runBenchmarkTests() {
    std::cout << "\n=== Benchmark Tests ===" << std::endl;
    
    // 测试不同k值的影响
    std::vector<unsigned char> test_data = generateTestData(100000);
    std::vector<uint32_t> codepoints = normalize_to_codepoints(test_data);
    
    std::cout << "Testing different k values:" << std::endl;
    for (int k = 1; k <= 5; ++k) {
        auto start = std::chrono::high_resolution_clock::now();
        std::unordered_set<uint64_t> kgram_set = build_kgram_set(codepoints, k);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "k=" << k << ": " << kgram_set.size() << " k-grams, " 
                  << duration.count() << " μs" << std::endl;
    }
    
    // 测试哈希函数性能
    std::cout << "\nTesting hash function performance:" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) {
        fnv1a64_hash_kgram(codepoints, i % (codepoints.size() - 2), 3);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "1,000,000 hash calculations: " << duration.count() << " ms" << std::endl;
}

int main() {
    try {
        runPerformanceTests();
        runBenchmarkTests();
        
        std::cout << "\n=== Performance Test Completed ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Performance test failed: " << e.what() << std::endl;
        return 1;
    }
}
