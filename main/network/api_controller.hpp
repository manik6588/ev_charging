#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

/* ===== C TYPES (ESP-IDF) ===== */
extern "C" {
#include "esp_err.h"
#include "esp_http_server.h"
}

/* ===== Forward Declarations ===== */
struct cJSON;

/* ================= API CONTROLLER ================= */

class ApiController
{
public:
    /* ===== Function Type ===== */
    using FunctionHandler = std::function<void(const std::vector<int>&)>;

    /* ===== Entry Points ===== */
    static std::string process(const std::string &body);

    static void handleWebSocket(const std::string &msg);

    static esp_err_t handleHttp(httpd_req_t *req);

private:
    /* ===== Core Handlers ===== */
    static std::string handleRead(const std::string &section);
    static std::string handleUpdate(const std::string &section, cJSON *json);

    /* ===== Execution ===== */
    static void executeFunction(
        const std::string &section,
        const std::string &name,
        const std::vector<int> &args
    );

    /* ===== Section Maps ===== */
    static std::map<
        std::string,
        std::map<std::string, FunctionHandler>
    > sectionMaps;
};