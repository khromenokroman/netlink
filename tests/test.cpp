#include <gtest/gtest.h>

#include "../server/server.hpp"

// Дружественный тестовый класс
namespace tests {
class ServerTest_Friend {
   public:
    static nlohmann::json test_process_request(netlink::server::Server &server, const std::string &request) {
        return server.process_request(request); // Доступ к private-методу
    }
};
} // namespace tests

// Тест: Проверка корректной обработки действия "add"
TEST(ServerTests, ProcessValidAddRequest) {
    netlink::server::Server server;

    std::string valid_request = R"({"action": "add", "arg1": 3, "arg2": 5})";
    nlohmann::json expected_response = {{"result", 8}};

    auto response = tests::ServerTest_Friend::test_process_request(server, valid_request);
    EXPECT_EQ(response, expected_response);
}

// Тест: Проверка корректной обработки действия "sub"
TEST(ServerTests, ProcessValidSubRequest) {
    netlink::server::Server server;

    std::string valid_request = R"({"action": "sub", "arg1": 10, "arg2": 3})";
    nlohmann::json expected_response = {{"result", 7}};

    auto response = tests::ServerTest_Friend::test_process_request(server, valid_request);
    EXPECT_EQ(response, expected_response);
}

// Тест: Проверка корректной обработки действия "mul"
TEST(ServerTests, ProcessValidMulRequest) {
    netlink::server::Server server;

    std::string valid_request = R"({"action": "mul", "arg1": 4, "arg2": 5})";
    nlohmann::json expected_response = {{"result", 20}};

    auto response = tests::ServerTest_Friend::test_process_request(server, valid_request);
    EXPECT_EQ(response, expected_response);
}

// Тест: Некорректный запрос — отсутствует "action"
TEST(ServerTests, ProcessMissingActionField) {
    netlink::server::Server server;

    std::string invalid_request = R"({"arg1": 3, "arg2": 5})";

    EXPECT_THROW(
        tests::ServerTest_Friend::test_process_request(server, invalid_request),
        std::runtime_error
    );
}

// Тест: Некорректное действие (неизвестное значение "action")
TEST(ServerTests, ProcessUnknownAction) {
    netlink::server::Server server;

    std::string invalid_request = R"({"action": "div", "arg1": 10, "arg2": 2})";

    EXPECT_THROW(
        tests::ServerTest_Friend::test_process_request(server, invalid_request),
        std::runtime_error
    );
}

// Тест: Некорректный JSON (синтаксическая ошибка)
TEST(ServerTests, ProcessInvalidJson) {
    netlink::server::Server server;

    std::string invalid_request = R"({"action": "add", "arg1": 3, "arg2": )"; // Неполный JSON

    EXPECT_THROW(
        tests::ServerTest_Friend::test_process_request(server, invalid_request),
        nlohmann::json::parse_error
    );
}

// Тест: Некорректные аргументы (отсутствует "arg1")
TEST(ServerTests, ProcessMissingArg1Field) {
    netlink::server::Server server;

    std::string invalid_request = R"({"action": "add", "arg2": 5})";

    EXPECT_THROW(
        tests::ServerTest_Friend::test_process_request(server, invalid_request),
        std::runtime_error
    );
}

// Тест: Некорректные аргументы (отсутствует "arg2")
TEST(ServerTests, ProcessMissingArg2Field) {
    netlink::server::Server server;

    std::string invalid_request = R"({"action": "add", "arg1": 3})";

    EXPECT_THROW(
        tests::ServerTest_Friend::test_process_request(server, invalid_request),
        std::runtime_error
    );
}

// Тест: Неподдерживаемый тип аргументов (строка вместо числа)
TEST(ServerTests, ProcessInvalidArgType) {
    netlink::server::Server server;

    std::string invalid_request = R"({"action": "add", "arg1": "three", "arg2": 5})";

    EXPECT_THROW(
        tests::ServerTest_Friend::test_process_request(server, invalid_request),
        nlohmann::json::type_error
    );
}

// Тест: Аргументы равны нулю (пограничный случай)
TEST(ServerTests, ProcessZeroArguments) {
    netlink::server::Server server;

    std::string valid_request = R"({"action": "add", "arg1": 0, "arg2": 0})";
    nlohmann::json expected_response = {{"result", 0}};

    auto response = tests::ServerTest_Friend::test_process_request(server, valid_request);
    EXPECT_EQ(response, expected_response);
}

// Тест: Аргументы с отрицательными числами
TEST(ServerTests, ProcessNegativeArguments) {
    netlink::server::Server server;

    std::string valid_request = R"({"action": "add", "arg1": -3, "arg2": -5})";
    nlohmann::json expected_response = {{"result", -8}};

    auto response = tests::ServerTest_Friend::test_process_request(server, valid_request);
    EXPECT_EQ(response, expected_response);
}