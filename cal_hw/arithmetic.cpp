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
#include <algorithm>

// �����࣬���ڴ����������
class Fraction {
private:
    int numerator;   // ����
    int denominator; // ��ĸ
    int integer;     // �������֣����ڴ�������

    // �������Լ��
    int gcd(int a, int b) {
        return b == 0 ? a : gcd(b, a % b);
    }

    // �������
    void simplify() {
        if (numerator == 0) {
            denominator = 1;
            return;
        }
        int g = gcd(std::abs(numerator), denominator);
        numerator /= g;
        denominator /= g;

        // �����������ʽ
        if (std::abs(numerator) >= denominator) {
            integer += numerator / denominator;
            numerator = numerator % denominator;
        }
    }

public:
    Fraction(int n = 0, int d = 1, int i = 0) : numerator(n), denominator(d), integer(i) {
        if (denominator == 0) throw std::invalid_argument("��ĸ����Ϊ0");
        if (denominator < 0) {
            numerator = -numerator;
            denominator = -denominator;
        }
        simplify();
    }

    // �����ӷ�
    Fraction operator+(const Fraction& other) const {
        // ��������ת��Ϊ�ٷ������м���
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        int new_numerator = total1 * other.denominator + total2 * denominator;
        int new_denominator = denominator * other.denominator;
        return Fraction(new_numerator, new_denominator);
    }

    // ��������
    Fraction operator-(const Fraction& other) const {
        // ��������ת��Ϊ�ٷ������м���
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        int new_numerator = total1 * other.denominator - total2 * denominator;
        int new_denominator = denominator * other.denominator;
        return Fraction(new_numerator, new_denominator);
    }

    // �����˷�
    Fraction operator*(const Fraction& other) const {
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        return Fraction(total1 * total2, denominator * other.denominator);
    }

    // ��������
    Fraction operator/(const Fraction& other) const {
        if (other.integer == 0 && other.numerator == 0)
            throw std::invalid_argument("��������Ϊ0");
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        return Fraction(total1 * other.denominator, denominator * total2);
    }

    // �Ƚ������
    bool operator>=(const Fraction& other) const {
        int total1 = integer * denominator + numerator;
        int total2 = other.integer * other.denominator + other.numerator;
        return total1 * other.denominator >= total2 * denominator;
    }

    // ת��Ϊ�ַ���
    std::string toString() const {
        std::stringstream ss;
        if (integer != 0) {
            ss << integer;
            if (numerator != 0) {
                ss << "'" << numerator << "/" << denominator;
            }
        }
        else if (numerator == 0) {
            ss << "0";
        }
        else {
            ss << numerator << "/" << denominator;
        }
        return ss.str();
    }

    // ��ȡ����ֵ
    double getValue() const {
        return integer + (double)numerator / denominator;
    }
};

// ���ʽ��������
class ExpressionGenerator {
private:
    int range;
    std::mt19937 rng;
    std::set<std::string> usedExpressions;

    // �����������
    Fraction generateRandomInteger() {
        return Fraction(rng() % range);  // ����[0, range-1]��Χ�ڵ�����
    }

    // �����������
    Fraction generateRandomFraction() {
        // ʹ�ó����ļ򵥷�ĸ��2, 3, 4, 5, 6, 8, 10
        const int commonDenominators[] = { 2, 3, 4, 5, 6, 8, 10 };
        int d;

        // �����ΧС��10��ʹ�÷�Χ�ڵķ�ĸ
        if (range <= 10) {
            d = rng() % (range - 1) + 2;
        }
        else {
            // ����ʹ�ó�����ĸ
            d = commonDenominators[rng() % 7];
        }

        // ����������[1, ��ĸ-1]��Χ�ڣ�ȷ���������
        int n = (rng() % (d - 1)) + 1;

        // �������������ڽ�С��Χ�ڣ�ʹ�������
        int i = rng() % std::min(5, range / 2);

        return Fraction(n, d, i);
    }

    // ����������������������
    Fraction generateRandomNumber() {
        // 60%��������������40%�������ɷ���
        return (rng() % 100 < 60) ? generateRandomInteger() : generateRandomFraction();
    }

    // ������������
    char generateRandomOperator() {
        const char ops[] = { '+', '-', '*', '/' };
        return ops[rng() % 4];
    }

    // ��׼�����ʽ���������ɺͽ����
    std::string normalizeExpression(const std::string& expr) {
        std::istringstream iss(expr);
        std::vector<std::string> tokens;
        std::string token;

        // �ִ�
        while (iss >> token) {
            if (token != "=") tokens.push_back(token);
        }

        // ���ֻ��һ������ֱ�ӷ���
        if (tokens.size() <= 1) return expr;

        // ���������������
        if (tokens.size() == 3) {
            Fraction num1 = parseFraction(tokens[0]);
            char op = tokens[1][0];
            Fraction num2 = parseFraction(tokens[2]);

            // ���ڼӷ��ͳ˷���ȷ����С������ǰ��
            if ((op == '+' || op == '*') && num2.getValue() < num1.getValue()) {
                return num2.toString() + " " + op + " " + num1.toString();
            }
            return expr;
        }

        // ���������������
        if (tokens.size() == 5) {
            Fraction num1 = parseFraction(tokens[0]);
            char op1 = tokens[1][0];
            Fraction num2 = parseFraction(tokens[2]);
            char op2 = tokens[3][0];
            Fraction num3 = parseFraction(tokens[4]);

            // ��������������ͬ�����㽻���ɣ��ӷ���˷���
            if ((op1 == op2) && (op1 == '+' || op1 == '*')) {
                std::vector<Fraction> nums = { num1, num2, num3 };
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

    // ���������ַ���
    Fraction parseFraction(const std::string& str) {
        size_t quotePos = str.find('\'');
        size_t slashPos = str.find('/');

        if (quotePos == std::string::npos && slashPos == std::string::npos) {
            return Fraction(std::stoi(str));
        }
        else if (quotePos == std::string::npos) {
            int num = std::stoi(str.substr(0, slashPos));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den);
        }
        else {
            int integer = std::stoi(str.substr(0, quotePos));
            int num = std::stoi(str.substr(quotePos + 1, slashPos - quotePos - 1));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den, integer);
        }
    }

    // ������ʽ�����������������ȼ���
    Fraction calculateExpression(const std::string& expr) {
        std::istringstream iss(expr);
        std::vector<std::string> tokens;
        std::string token;

        while (iss >> token) {
            if (token != "=") tokens.push_back(token);
        }

        // ����˳�
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

        // ����Ӽ�
        Fraction result = parseFraction(tokens[0]);
        for (size_t i = 1; i < tokens.size(); i += 2) {
            Fraction operand = parseFraction(tokens[i + 1]);
            if (tokens[i] == "+") {
                result = result + operand;
            }
            else if (tokens[i] == "-") {
                result = result - operand;
            }
        }

        return result;
    }

public:
    ExpressionGenerator(int r) : range(r) {
        std::random_device rd;
        rng.seed(rd());
    }

    // ����һ���µı��ʽ
    std::pair<std::string, Fraction> generateExpression() {
        // ����1-3�����������Ŀ
        int operatorCount = (rng() % 3) + 1;  // �������1-3�������
        std::vector<Fraction> numbers;
        std::vector<char> operators;
        std::string expression;
        Fraction result(0);

        // ���ɵ�һ����
        numbers.push_back(generateRandomNumber());
        result = numbers[0];

        // ����������ͺ�������
        for (int i = 0; i < operatorCount; ++i) {
            char op;
            Fraction nextNum;
            bool validExpr = false;
            int attempts = 0;  // ��ӳ��Դ�������

            // ����������Ч�ı��ʽ
            while (!validExpr && attempts < 50) {  // ���ӳ��Դ���
                attempts++;

                // �Ż������ѡ�����ӼӼ������ʣ����ٳ˳�������
                // 70%�Ӽ�����30%�˳���
                if (rng() % 100 < 70) {
                    // �Ӽ�����60%�ӷ���40%����
                    op = (rng() % 100 < 60) ? '+' : '-';
                }
                else {
                    // �˳�����50%�˷���50%����
                    op = (rng() % 2 == 0) ? '*' : '/';
                }

                // ���������ѡ����һ����
                nextNum = (op == '*' || op == '/') ? generateRandomInteger() : generateRandomNumber();

                operators.push_back(op);
                numbers.push_back(nextNum);

                // ������ʱ���ʽ
                std::string tempExpr = numbers[0].toString();
                for (size_t j = 0; j < operators.size(); ++j) {
                    tempExpr += " " + std::string(1, operators[j]) + " " + numbers[j + 1].toString();
                }

                // ʹ��calculateExpression������ʱ���
                try {
                    Fraction tempResult = calculateExpression(tempExpr);
                    if (tempResult.getValue() >= 0 && tempResult.getValue() < range) {
                        validExpr = true;
                    }
                    else {
                        // ������ڷ�Χ�ڣ�����
                        operators.pop_back();
                        numbers.pop_back();
                    }
                }
                catch (...) {
                    // �����������
                    operators.pop_back();
                    numbers.pop_back();
                }
            }

            // ����޷�������Ч���ʽ������������������¿�ʼ
            if (!validExpr) {
                operatorCount = i;
                break;
            }
        }

        // �������ʽ�ַ���
        expression = numbers[0].toString();
        for (size_t i = 0; i < operators.size(); ++i) {
            expression += " " + std::string(1, operators[i]) + " " + numbers[i + 1].toString();
        }

        // ʹ��calculateExpression�������ս��
        result = calculateExpression(expression);

        // ����Ƿ����ظ��ı��ʽ
        std::string normalizedExpr = normalizeExpression(expression);

        // ���ɱ��ʽ�����п��ܱ��壨�������ɣ�
        std::vector<std::string> variants;
        if (operators.size() == 2) {
            // ������������������������Ƿ����Ӧ�ý����
            if ((operators[0] == '+' && operators[1] == '+') ||
                (operators[0] == '*' && operators[1] == '*')) {
                // ���� (a+b)+c �� a+(b+c) �ı�׼��ʽ
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

        // ������б����Ƿ�����ظ�
        bool isDuplicate = false;
        if (!variants.empty()) {
            for (const auto& variant : variants) {
                if (usedExpressions.find(variant) != usedExpressions.end()) {
                    isDuplicate = true;
                    break;
                }
            }
        }
        else {
            isDuplicate = usedExpressions.find(normalizedExpr) != usedExpressions.end();
        }

        if (isDuplicate) {
            return generateExpression();  // ������ظ��ģ���������
        }

        // �������б���
        usedExpressions.insert(normalizedExpr);
        for (const auto& variant : variants) {
            usedExpressions.insert(variant);
        }

        return { expression, result };
    }
};

// ��������
class ArithmeticApp {
private:
    ExpressionGenerator* generator;
    std::vector<std::pair<std::string, Fraction>> exercises;

    // ���ַ���ת��Ϊ����
    Fraction parseFraction(const std::string& str) {
        size_t quotePos = str.find('\'');
        size_t slashPos = str.find('/');

        if (quotePos == std::string::npos && slashPos == std::string::npos) {
            // ����
            return Fraction(std::stoi(str));
        }
        else if (quotePos == std::string::npos) {
            // �����
            int num = std::stoi(str.substr(0, slashPos));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den);
        }
        else {
            // ������
            int integer = std::stoi(str.substr(0, quotePos));
            int num = std::stoi(str.substr(quotePos + 1, slashPos - quotePos - 1));
            int den = std::stoi(str.substr(slashPos + 1));
            return Fraction(num, den, integer);
        }
    }

    // ������ʽ���
    Fraction calculateExpression(const std::string& expr) {
        std::istringstream iss(expr);
        std::vector<std::string> tokens;
        std::string token;

        // �ִ�
        while (iss >> token) {
            if (token != "=") {
                tokens.push_back(token);
            }
        }

        // ��������
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

        // ����˳�
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

        // ����Ӽ�
        Fraction result = parseFraction(tokens[0]);
        for (size_t i = 1; i < tokens.size(); i += 2) {
            Fraction operand = parseFraction(tokens[i + 1]);
            if (tokens[i] == "+") {
                result = result + operand;
            }
            else if (tokens[i] == "-") {
                result = result - operand;
            }
        }

        return result;
    }

public:
    ArithmeticApp() : generator(nullptr) {}
    ~ArithmeticApp() { delete generator; }

    // ���������в���
    bool parseArguments(int argc, char* argv[]) {
        if (argc < 2) return false;

        std::string mode = argv[1];
        if (mode == "-n") {
            // ����������Ŀ�����
            int count = 10;  // Ĭ������10����Ŀ
            int range = 0;   // ��Χ������ʽָ��

            // ��������
            for (int i = 2; i < argc; i++) {
                std::string arg = argv[i];
                if (arg == "-r" && i + 1 < argc) {
                    try {
                        range = std::stoi(argv[++i]);
                    }
                    catch (const std::exception&) {
                        return false;
                    }
                }
                else if (arg[0] != '-') {
                    try {
                        count = std::stoi(arg);
                    }
                    catch (const std::exception&) {
                        return false;
                    }
                }
            }

            // ��֤����
            if (count <= 0 || count > 10000 || range <= 0) {
                return false;
            }

            generateExercises(count, range);
            saveToFiles();
        }
        else if (mode == "-e") {
            if (argc != 5 || std::string(argv[3]) != "-a") return false;
            verifyAnswers(argv[2], argv[4]);
        }
        else {
            return false;
        }
        return true;
    }

    // ������Ŀ
    void generateExercises(int count, int range) {
        generator = new ExpressionGenerator(range);
        exercises.clear();

        for (int i = 0; i < count; ++i) {
            auto exercise = generator->generateExpression();
            exercises.push_back(exercise);
        }
    }

    // ������Ŀ�ʹ�
    void saveToFiles() {
        std::ofstream exerciseFile("Exercises.txt");
        std::ofstream answerFile("Answers.txt");

        if (!exerciseFile || !answerFile) {
            throw std::runtime_error("�޷���������ļ�");
        }

        // ���������ʽ
        for (size_t i = 0; i < exercises.size(); ++i) {
            // ����Ŀ�ļ���������
            exerciseFile << i + 1 << ". " << exercises[i].first << " =" << std::endl;

            // �ڴ��ļ���������
            answerFile << i + 1 << ". " << exercises[i].second.toString() << std::endl;
        }

        exerciseFile.close();
        answerFile.close();
    }

    // ��֤��
    void verifyAnswers(const std::string& exerciseFile, const std::string& answerFile) {
        std::ifstream exercises(exerciseFile);
        std::ifstream answers(answerFile);
        std::ofstream grade("Grade.txt");

        if (!exercises || !answers || !grade) {
            throw std::runtime_error("�޷����ļ�");
        }

        std::vector<int> correct, wrong;
        std::string line, answer;
        int index = 1;

        while (std::getline(exercises, line) && std::getline(answers, answer)) {
            try {
                // ����Ŀ������ȡ���ʽ��ȥ����ź͵Ⱥţ�
                size_t dotPos = line.find('.');
                if (dotPos != std::string::npos) {
                    line = line.substr(dotPos + 1);
                }
                size_t equalPos = line.find('=');
                if (equalPos != std::string::npos) {
                    line = line.substr(0, equalPos);
                }

                // �Ӵ�������ȡ�𰸣�ȥ����ţ�
                dotPos = answer.find('.');
                if (dotPos != std::string::npos) {
                    answer = answer.substr(dotPos + 1);
                }
                // ȥ����β�ո�
                answer.erase(0, answer.find_first_not_of(" \t"));
                answer.erase(answer.find_last_not_of(" \t") + 1);

                Fraction calculatedResult = calculateExpression(line);
                Fraction providedAnswer = parseFraction(answer);

                // �Ƚϼ��������ṩ�Ĵ�
                if (std::abs(calculatedResult.getValue() - providedAnswer.getValue()) < 1e-6) {
                    correct.push_back(index);
                }
                else {
                    wrong.push_back(index);
                }
            }
            catch (const std::exception&) {
                // ���������̳������Ϊ�����
                wrong.push_back(index);
            }
            index++;
        }

        // ����Ҫ���ʽ������
        if (!correct.empty()) {
            grade << "Correct: " << correct.size() << " (";
            for (size_t i = 0; i < correct.size(); ++i) {
                grade << correct[i];
                if (i < correct.size() - 1) grade << ", ";
            }
            grade << ")" << std::endl;
        }
        else {
            grade << "Correct: 0" << std::endl;
        }

        if (!wrong.empty()) {
            grade << "Wrong: " << wrong.size() << " (";
            for (size_t i = 0; i < wrong.size(); ++i) {
                grade << wrong[i];
                if (i < wrong.size() - 1) grade << ", ";
            }
            grade << ")" << std::endl;
        }
        else {
            grade << "Wrong: 0" << std::endl;
        }

        exercises.close();
        answers.close();
        grade.close();
    }
};

void printHelp(const char* programName) {
    std::cout << "Сѧ����������Ŀ���ɳ���\n\n"
        << "�÷�:\n"
        << "1. ������Ŀ��\n"
        << "   " << programName << " -n [��Ŀ����] -r <��ֵ��Χ>\n"
        << "   ���磺\n"
        << "   ����20����Ŀ��" << programName << " -n 20 -r 100\n\n"
        << "2. ��֤�𰸣�\n"
        << "   " << programName << " -e <��Ŀ�ļ�> -a <���ļ�>\n"
        << "   ���磺" << programName << " -e Exercises.txt -a Answers.txt\n\n"
        << "����˵����\n"
        << "  -n: ������Ŀģʽ������ɸ���Ŀ������1-10000֮�䣩��Ĭ������10����Ŀ\n"
        << "  -r: ��ֵ��Χ������ָ��������0��������\n"
        << "  -e: ��Ŀ�ļ�·��\n"
        << "  -a: ���ļ�·��\n\n";
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        printHelp(argv[0]);
        return 0;
    }

    ArithmeticApp app;
    try {
        if (!app.parseArguments(argc, argv)) {
            std::cerr << "���󣺲�����ʽ����ȷ\n\n";
            printHelp(argv[0]);
            return 1;
        }
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "��������" << e.what() << std::endl;
        return 1;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "����ʱ����" << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "�������" << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "����δ֪����" << std::endl;
        return 1;
    }

    std::cout << "����ִ�гɹ���" << std::endl;
    return 0;
}