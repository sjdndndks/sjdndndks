#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <set>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

// 分数类，用于处理分数运算
class Fraction {
private:
    int numerator;   // 分子
    int denominator; // 分母
    int integer;     // 整数部分（用于带分数）

    // 计算最大公约数
    int gcd(int a, int b) {
        return b == 0 ? a : gcd(b, a % b);
    }

    // 化简分数
    void simplify() {
        if (numerator == 0) {
            denominator = 1;
            return;
        }
        int g = gcd(std::abs(numerator), denominator);
        numerator /= g;
        denominator /= g;
        
        // 处理带分数形式
        if (std::abs(numerator) >= denominator) {
            integer += numerator / denominator;
            numerator = std::abs(numerator) % denominator;
            if (integer < 0) numerator = -numerator;
        }
    }

public:
    Fraction(int n = 0, int d = 1, int i = 0) : numerator(n), denominator(d), integer(i) {
        if (denominator == 0) throw std::invalid_argument("分母不能为0");
        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
        simplify();
    }

    // 分数加法
    Fraction operator+(const Fraction& other) const {
        int new_integer = integer + other.integer;
        int new_numerator = numerator * other.denominator + other.numerator * denominator;
        int new_denominator = denominator * other.denominator;
        return Fraction(new_numerator, new_denominator, new_integer);
    }

    // 分数减法
    Fraction operator-(const Fraction& other) const {
        int new_integer = integer - other.integer;
        int new_numerator = numerator * other.denominator - other.numerator * denominator;
        int new_denominator = denominator * other.denominator;
        return Fraction(new_numerator, new_denominator, new_integer);
    }

    // 分数乘法
    Fraction operator*(const Fraction& other) const {
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        return Fraction(total1 * total2, denominator * other.denominator);
    }

    // 分数除法
    Fraction operator/(const Fraction& other) const {
        if (other.integer == 0 && other.numerator == 0)
            throw std::invalid_argument("除数不能为0");
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        return Fraction(total1 * other.denominator, denominator * total2);
    }

    // 比较运算符
    bool operator>=(const Fraction& other) const {
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        return total1 * other.denominator >= total2 * denominator;
    }

    // 转换为字符串
    std::string toString() const {
        std::stringstream ss;
        if (integer != 0) {
            ss << integer;
            if (numerator != 0) ss << "'" << numerator << "/" << denominator;
        } else if (numerator == 0) {
            ss << "0";
        } else {
            ss << numerator << "/" << denominator;
        }
        return ss.str();
    }

    // 获取分数值
    double getValue() const {
        return integer + (double)numerator / denominator;
    }
};

// 表达式生成器类
class ExpressionGenerator {
private:
    int range;
    std::mt19937 rng;
    std::set<std::string> usedExpressions;

    // 生成随机整数
    Fraction generateRandomInteger() {
        return Fraction(rng() % range);  // 生成[0, range-1]范围内的整数
    }

    // 生成随机分数
    Fraction generateRandomFraction() {
        // 使用常见的简单分母：2, 3, 4, 5, 6, 8, 10
        const int commonDenominators[] = {2, 3, 4, 5, 6, 8, 10};
        int d;
        
        // 如果范围小于10，使用范围内的分母
        if (range <= 10) {
            d = rng() % (range - 1) + 2;
        } else {
            // 否则使用常见分母
            d = commonDenominators[rng() % 7];
        }
        
        // 分子限制在[1, 分母-1]范围内，确保是真分数
        int n = (rng() % (d - 1)) + 1;
        
        // 整数部分限制在较小范围内，使计算更简单
        int i = rng() % std::min(5, range / 2);
        
        return Fraction(n, d, i);
    }

    // 生成随机数（整数或分数）
    Fraction generateRandomNumber() {
        // 60%概率生成整数，40%概率生成分数
        return (rng() % 100 < 60) ? generateRandomInteger() : generateRandomFraction();
    }

    // 生成随机运算符
    char generateRandomOperator() {
        const char ops[] = {'+', '-', '*', '/'};
        return ops[rng() % 4];
    }

    // 标准化表达式，处理交换律和结合律
    std::string normalizeExpression(const std::string& expr) {
        std::istringstream iss(expr);
        std::vector<std::string> tokens;
        std::string token;
        
        // 分词
        while (iss >> token) {
            if (token != "=") tokens.push_back(token);
        }

        // 如果只有一个数，直接返回
        if (tokens.size() <= 1) return expr;

        // 处理两个数的情况
        if (tokens.size() == 3) {
            Fraction num1 = parseFraction(tokens[0]);
            char op = tokens[1][0];
            Fraction num2 = parseFraction(tokens[2]);

            // 对于加法和乘法，确保较小的数在前面
            if ((op == '+' || op == '*') && num2.getValue() < num1.getValue()) {
                return num2.toString() + " " + op + " " + num1.toString();
            }
            return expr;
        }

        // 处理三个数的情况
        if (tokens.size() == 5) {
            Fraction num1 = parseFraction(tokens[0]);
            char op1 = tokens[1][0];
            Fraction num2 = parseFraction(tokens[2]);
            char op2 = tokens[3][0];
            Fraction num3 = parseFraction(tokens[4]);

            // 如果两个运算符相同且满足交换律（加法或乘法）
            if ((op1 == op2) && (op1 == '+' || op1 == '*')) {
                std::vector<Fraction> nums = {num1, num2, num3};
                std::sort(nums.begin(), nums.end(), 
                    [](const Fraction& a, const Fraction& b) {
                        return a.getValue() < b.getValue();
                    });
                return nums[0].toString() + " " + op1 + " " + 
                       nums[1].toString() + " " + op2 + " " + 
                       nums[2].toString();
            }
        }

        return expr;
    }

    // 解析分数字符串
    Fraction parseFraction(const std::string& str) {
        size_t quotePos = str.find('\'');
        size_t slashPos = str.find('/');
        
        if (quotePos == std::string::npos && slashPos == std::string::npos) {
            return Fraction(std::stoi(str));
        } else if (quotePos == std::string::npos) {
            int num = std::stoi(str.substr(0, slashPos));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den);
        } else {
            int integer = std::stoi(str.substr(0, quotePos));
            int num = std::stoi(str.substr(quotePos + 1, slashPos - quotePos - 1));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den, integer);
        }
    }

public:
    ExpressionGenerator(int r) : range(r) {
        std::random_device rd;
        rng.seed(rd());
    }

    // 生成一个新的表达式
    std::pair<std::string, Fraction> generateExpression() {
        int operatorCount = (rng() % 3) + 1;  // 1-3个运算符
        std::vector<Fraction> numbers;
        std::vector<char> operators;
        std::string expression;
        Fraction result(0);

        // 生成第一个数
        numbers.push_back(generateRandomNumber());
        result = numbers[0];

        // 生成运算符和后续的数
        for (int i = 0; i < operatorCount; ++i) {
            char op;
            Fraction nextNum;
            bool validExpr = false;
            int attempts = 0;  // 添加尝试次数限制

            // 尝试生成有效的表达式
            while (!validExpr && attempts < 10) {  // 限制尝试次数
                attempts++;
                
                // 根据前一个结果的类型调整运算符选择
                bool isResultInteger = (result.getValue() == (int)result.getValue());
                if (isResultInteger) {
                    // 如果当前结果是整数，增加生成加减法的概率
                    const char ops[] = {'+', '+', '-', '*', '/'};
                    op = ops[rng() % 5];
                } else {
                    // 如果当前结果是分数，增加生成乘除法的概率
                    const char ops[] = {'+', '-', '*', '*', '/'};
                    op = ops[rng() % 5];
                }

                // 根据运算符选择下一个数
                nextNum = (op == '*' || op == '/') ? generateRandomInteger() : generateRandomNumber();
                
                try {
                    Fraction tempResult;
                    switch (op) {
                        case '+':
                        tempResult = result + nextNum;
                        if (tempResult.getValue() < range && tempResult.getValue() >= 0) validExpr = true;
                        break;
                    case '-':
                        tempResult = result - nextNum;
                        if (tempResult.getValue() >= 0) validExpr = true;
                        break;
                        case '*':
                                tempResult = result * nextNum;
                                if (tempResult.getValue() < range && tempResult.getValue() >= 0) validExpr = true;
                                break;
                            case '/':
                                if (nextNum.getValue() != 0) {
                                    tempResult = result / nextNum;
                                    // 确保除法结果是真分数且在范围内
                                    if (tempResult.getValue() < 1 && tempResult.getValue() > 0) {
                                        validExpr = true;
                                    }
                                }
                                break;
                    }
                    if (validExpr) result = tempResult;
                } catch (...) {
                    continue;
                }
            }
            
            // 如果无法生成有效表达式，减少运算符数量重新开始
            if (!validExpr) {
                operatorCount = i;
                break;
            }

            operators.push_back(op);
            numbers.push_back(nextNum);
        }

        // 构建表达式字符串
        expression = numbers[0].toString();
        for (size_t i = 0; i < operators.size(); ++i) {
            expression += " " + std::string(1, operators[i]) + " " + numbers[i + 1].toString();
        }

        // 检查是否是重复的表达式
        std::string normalizedExpr = normalizeExpression(expression);
        
        // 生成表达式的所有可能变体（处理结合律）
        std::vector<std::string> variants;
        if (operators.size() == 2) {
            // 对于两个运算符的情况，检查是否可以应用结合律
            if ((operators[0] == '+' && operators[1] == '+') ||
                (operators[0] == '*' && operators[1] == '*')) {
                // 生成 (a+b)+c 和 a+(b+c) 的标准形式
                std::string variant1 = numbers[0].toString() + " " + operators[0] + " " + 
                                     numbers[1].toString() + " " + operators[1] + " " + 
                                     numbers[2].toString();
                std::string variant2 = numbers[1].toString() + " " + operators[1] + " " + 
                                     numbers[2].toString() + " " + operators[0] + " " + 
                                     numbers[0].toString();
                variants.push_back(normalizeExpression(variant1));
                variants.push_back(normalizeExpression(variant2));
            }
        }
        
        // 检查所有变体是否存在重复
        bool isDuplicate = false;
        if (!variants.empty()) {
            for (const auto& variant : variants) {
                if (usedExpressions.find(variant) != usedExpressions.end()) {
                    isDuplicate = true;
                    break;
                }
            }
        } else {
            isDuplicate = usedExpressions.find(normalizedExpr) != usedExpressions.end();
        }
        
        if (isDuplicate) {
            return generateExpression();  // 如果是重复的，重新生成
        }
        
        // 保存所有变体
        usedExpressions.insert(normalizedExpr);
        for (const auto& variant : variants) {
            usedExpressions.insert(variant);
        }
        
        return {expression, result};
    }
};

// 主程序类
class ArithmeticApp {
private:
    ExpressionGenerator* generator;
    std::vector<std::pair<std::string, Fraction>> exercises;

    // 将字符串转换为分数
    Fraction parseFraction(const std::string& str) {
        size_t quotePos = str.find('\'');
        size_t slashPos = str.find('/');
        
        if (quotePos == std::string::npos && slashPos == std::string::npos) {
            // 整数
            return Fraction(std::stoi(str));
        } else if (quotePos == std::string::npos) {
            // 真分数
            int num = std::stoi(str.substr(0, slashPos));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den);
        } else {
            // 带分数
            int integer = std::stoi(str.substr(0, quotePos));
            int num = std::stoi(str.substr(quotePos + 1, slashPos - quotePos - 1));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den, integer);
        }
    }

    // 计算表达式结果
    Fraction calculateExpression(const std::string& expr) {
        std::istringstream iss(expr);
        std::vector<std::string> tokens;
        std::string token;
        
        // 分词
        while (iss >> token) {
            if (token != "=") {
                tokens.push_back(token);
            }
        }

        // 处理括号
        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == "(") {
                size_t j = i + 1;
                int bracketCount = 1;
                std::string subExpr;
                while (j < tokens.size() && bracketCount > 0) {
                    if (tokens[j] == "(") bracketCount++;
                    if (tokens[j] == ")") bracketCount--;
                    if (bracketCount > 0) {
                        subExpr += tokens[j] + " ";
                    }
                    j++;
                }
                if (bracketCount == 0) {
                    Fraction result = calculateExpression(subExpr);
                    tokens[i] = result.toString();
                    tokens.erase(tokens.begin() + i + 1, tokens.begin() + j);
                }
            }
        }

        // 计算乘除
        for (size_t i = 1; i < tokens.size(); i += 2) {
            if (tokens[i] == "*" || tokens[i] == "/") {
                Fraction left = parseFraction(tokens[i - 1]);
                Fraction right = parseFraction(tokens[i + 1]);
                Fraction result = (tokens[i] == "*") ? left * right : left / right;
                tokens[i - 1] = result.toString();
                tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
                i -= 2;
            }
        }

        // 计算加减
        Fraction result = parseFraction(tokens[0]);
        for (size_t i = 1; i < tokens.size(); i += 2) {
            Fraction operand = parseFraction(tokens[i + 1]);
            if (tokens[i] == "+") {
                result = result + operand;
            } else if (tokens[i] == "-") {
                result = result - operand;
            }
        }

        return result;
    }

public:
    ArithmeticApp() : generator(nullptr) {}
    ~ArithmeticApp() { delete generator; }

    // 解析命令行参数
    bool parseArguments(int argc, char* argv[]) {
        if (argc < 2) return false;

        std::string mode = argv[1];
        if (mode == "-n") {
            // 处理生成题目的情况
            int count = 10;  // 默认生成10道题目
            int range = 0;   // 范围必须显式指定
            
            // 解析参数
            for (int i = 2; i < argc; i++) {
                std::string arg = argv[i];
                if (arg == "-r" && i + 1 < argc) {
                    try {
                        range = std::stoi(argv[++i]);
                    } catch (const std::exception&) {
                        return false;
                    }
                } else if (arg[0] != '-') {
                    try {
                        count = std::stoi(arg);
                    } catch (const std::exception&) {
                        return false;
                    }
                }
            }
            
            // 验证参数
            if (count <= 0 || count > 10000 || range <= 0) {
                return false;
            }
            
            generateExercises(count, range);
            saveToFiles();
        } else if (mode == "-e") {
            if (argc != 5 || std::string(argv[3]) != "-a") return false;
            verifyAnswers(argv[2], argv[4]);
        } else {
            return false;
        }
        return true;
    }

    // 生成题目
    void generateExercises(int count, int range) {
        generator = new ExpressionGenerator(range);
        exercises.clear();

        for (int i = 0; i < count; ++i) {
            auto exercise = generator->generateExpression();
            exercises.push_back(exercise);
        }
    }

    // 保存题目和答案
    void saveToFiles() {
        std::ofstream exerciseFile("Exercises.txt");
        std::ofstream answerFile("Answers.txt");

        if (!exerciseFile || !answerFile) {
            throw std::runtime_error("无法创建输出文件");
        }

        // 设置输出格式
        for (size_t i = 0; i < exercises.size(); ++i) {
            // 在题目文件中添加题号
            exerciseFile << i + 1 << ". " << exercises[i].first << " =" << std::endl;
            
            // 在答案文件中添加题号
            answerFile << i + 1 << ". " << exercises[i].second.toString() << std::endl;
        }

        exerciseFile.close();
        answerFile.close();
    }

    // 验证答案
    void verifyAnswers(const std::string& exerciseFile, const std::string& answerFile) {
        std::ifstream exercises(exerciseFile);
        std::ifstream answers(answerFile);
        std::ofstream grade("Grade.txt");

        if (!exercises || !answers || !grade) {
            throw std::runtime_error("无法打开文件");
        }

        std::vector<int> correct, wrong;
        std::string line, answer;
        int index = 1;

        while (std::getline(exercises, line) && std::getline(answers, answer)) {
            try {
                Fraction calculatedResult = calculateExpression(line);
                Fraction providedAnswer = parseFraction(answer);
                
                // 比较计算结果和提供的答案
                if (std::abs(calculatedResult.getValue() - providedAnswer.getValue()) < 1e-6) {
                    correct.push_back(index);
                } else {
                    wrong.push_back(index);
                }
            } catch (const std::exception&) {
                // 如果计算过程出错，标记为错误答案
                wrong.push_back(index);
            }
            index++;
        }

        // 按照要求格式输出结果
        if (!correct.empty()) {
            grade << "Correct: " << correct.size() << " (";
            for (size_t i = 0; i < correct.size(); ++i) {
                grade << correct[i];
                if (i < correct.size() - 1) grade << ", ";
            }
            grade << ")" << std::endl;
        } else {
            grade << "Correct: 0" << std::endl;
        }

        if (!wrong.empty()) {
            grade << "Wrong: " << wrong.size() << " (";
            for (size_t i = 0; i < wrong.size(); ++i) {
                grade << wrong[i];
                if (i < wrong.size() - 1) grade << ", ";
            }
            grade << ")" << std::endl;
        } else {
            grade << "Wrong: 0" << std::endl;
        }

        exercises.close();
        answers.close();
        grade.close();
    }
};

void printHelp(const char* programName) {
    std::cout << "小学四则运算题目生成程序\n\n"
              << "用法:\n"
              << "1. 生成题目：\n"
              << "   " << programName << " -n [题目数量] -r <数值范围>\n"
              << "   例如：\n"
              << "   生成20道题目：" << programName << " -n 20 -r 100\n\n"
              << "2. 验证答案：\n"
              << "   " << programName << " -e <题目文件> -a <答案文件>\n"
              << "   例如：" << programName << " -e Exercises.txt -a Answers.txt\n\n"
              << "参数说明：\n"
              << "  -n: 生成题目模式，后面可跟题目数量（1-10000之间），默认生成10道题目\n"
              << "  -r: 数值范围（必须指定，大于0的整数）\n"
              << "  -e: 题目文件路径\n"
              << "  -a: 答案文件路径\n\n";
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printHelp(argv[0]);
        return 0;
    }

    ArithmeticApp app;
    try {
        if (!app.parseArguments(argc, argv)) {
            std::cerr << "错误：参数格式不正确\n\n";
            printHelp(argv[0]);
            return 1;
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << "参数错误：" << e.what() << std::endl;
        return 1;
    } catch (const std::runtime_error& e) {
        std::cerr << "运行时错误：" << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "程序错误：" << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "发生未知错误" << std::endl;
        return 1;
    }

    std::cout << "程序执行成功！" << std::endl;
    return 0;
}