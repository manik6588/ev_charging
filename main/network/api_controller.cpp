#include "api_controller.hpp"
#include "websocket.hpp"
#include "search.hpp"

extern "C"
{
#include "esp_log.h"
#include "cJSON.h"
#include "motor.h"
#include "limit.h"
}

static const char *TAG = "API";

static bool limit_block_A(int speed)
{
    if (speed > 0 && x_max_pressed())
    {
        ESP_LOGW(TAG, "Blocked: X_MAX limit reached");
        return true;
    }

    if (speed < 0 && x_min_pressed())
    {
        ESP_LOGW(TAG, "Blocked: X_MIN limit reached");
        return true;
    }

    return false;
}

static bool limit_block_B(int speed)
{
    if (speed > 0 && y_max_pressed())
    {
        ESP_LOGW(TAG, "Blocked: Y_MAX limit reached");
        return true;
    }

    if (speed < 0 && y_min_pressed())
    {
        ESP_LOGW(TAG, "Blocked: Y_MIN limit reached");
        return true;
    }

    return false;
}

void safety_monitor_task(void *arg)
{
    while (1)
    {
        uint8_t state = limit_get_state();

        if (state != 0)
        {
            ESP_LOGW(TAG, "Limit triggered! Emergency stop.");
            motor_stop_all();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ================= SECTION MAPS ================= */

// -------- MOTOR --------
static std::map<std::string, ApiController::FunctionHandler> motorMap = {

    {"start", [](const std::vector<int> &)
     {
         ESP_LOGI(TAG, "Motor START");

         motor_init();
         motor_enable();
     }},

    {"stop", [](const std::vector<int> &)
     {
         ESP_LOGI(TAG, "Motor STOP");

         motor_stop_all();
         motor_disable();
     }},

    // -------- MOTOR MAP UPDATES --------
    {"setA", [](const std::vector<int> &args)
     {
         if (args.empty())
             return;
         int v = args[0];
         motor_enable();

         if (limit_block_A(v))
         {
             motor_stop_all();
             return;
         }

         // Use motorX (Motor A)
         if (v > 0)
             motorX_forward(v);
         else if (v < 0)
             motorX_backward(-v);
         else
             motor_stop_all();
     }},

    {"setB", [](const std::vector<int> &args)
     {
         if (args.empty())
             return;
         int v = args[0];
         motor_enable();

         if (limit_block_B(v))
         {
             motor_stop_all();
             return;
         }

         // Use motorY (Motor B)
         if (v > 0)
             motorY_forward(v);
         else if (v < 0)
             motorY_backward(-v);
         else
             motor_stop_all();
     }},
};

// -------- SEARCH --------
static std::map<std::string, ApiController::FunctionHandler> searchMap = {

    {"start", [](const std::vector<int> &)
     {
         ESP_LOGI(TAG, "Search START");

         search_init(); // ensure initialized
         search_start();
     }},

    {"stop", [](const std::vector<int> &)
     {
         ESP_LOGI(TAG, "Search STOP");

         search_stop();
         motor_stop_all(); // safety: stop motors too
     }},
};

// -------- SYSTEM --------
static std::map<std::string, ApiController::FunctionHandler> systemMap = {

    {"reboot", [](const std::vector<int> &)
     {
         ESP_LOGW(TAG, "System REBOOT");
         esp_restart();
     }}

};

/* ================= SECTION MAP ================= */

std::map<std::string,
         std::map<std::string, ApiController::FunctionHandler>>
    ApiController::sectionMaps = {
        {"motor", motorMap},
        {"search", searchMap},
        {"system", systemMap}};

/* ================= HTTP ================= */

esp_err_t ApiController::handleHttp(httpd_req_t *req)
{
    char buffer[512];

    int len = httpd_req_recv(req, buffer, sizeof(buffer) - 1);
    if (len <= 0)
        return ESP_FAIL;

    buffer[len] = '\0';

    std::string response = process(buffer);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response.c_str());

    return ESP_OK;
}

/* ================= WEBSOCKET ================= */

void ApiController::handleWebSocket(const std::string &msg)
{
    ESP_LOGI(TAG, "WS RX: %s", msg.c_str());

    std::string response = process(msg);

    WebSocketServer::broadcast(response);
}

/* ================= CORE PROCESS ================= */

std::string ApiController::process(const std::string &body)
{
    cJSON *json = cJSON_Parse(body.c_str());
    if (!json)
        return R"({"error":"invalid json"})";

    cJSON *method = cJSON_GetObjectItem(json, "method");
    cJSON *section = cJSON_GetObjectItem(json, "section");

    if (!cJSON_IsString(method) || !cJSON_IsString(section))
    {
        cJSON_Delete(json);
        return R"({"error":"invalid format"})";
    }

    std::string m = method->valuestring;
    std::string s = section->valuestring;

    std::string result;

    if (m == "READ")
    {
        result = handleRead(s);
    }
    else if (m == "UPDATE")
    {
        result = handleUpdate(s, json);
    }
    else
    {
        result = R"({"error":"invalid method"})";
    }

    cJSON_Delete(json);
    return result;
}

/* ================= READ ================= */

std::string ApiController::handleRead(const std::string &section)
{
    if (section == "motor")
    {
        return R"({"status":"ok","motor":"active"})";
    }
    else if (section == "station")
    {
        return R"({"status":"ok","station":"running"})";
    }

    return R"({"error":"unknown section"})";
}

/* ================= UPDATE ================= */

std::string ApiController::handleUpdate(const std::string &section, cJSON *json)
{
    cJSON *actions = cJSON_GetObjectItem(json, "action");

    if (!cJSON_IsArray(actions))
        return R"({"error":"invalid action"})";

    cJSON *item = nullptr;

    cJSON_ArrayForEach(item, actions)
    {
        cJSON *fname = cJSON_GetObjectItem(item, "function_name");
        cJSON *args = cJSON_GetObjectItem(item, "arguments");

        if (!cJSON_IsString(fname) || !cJSON_IsArray(args))
            continue;

        std::vector<int> parsedArgs;

        cJSON *arg = nullptr;
        cJSON_ArrayForEach(arg, args)
        {
            parsedArgs.push_back(arg->valueint);
        }

        executeFunction(section, fname->valuestring, parsedArgs);
    }

    return R"({"status":"updated"})";
}

/* ================= EXECUTION ================= */

void ApiController::executeFunction(
    const std::string &section,
    const std::string &name,
    const std::vector<int> &args)
{
    auto sec = sectionMaps.find(section);
    if (sec == sectionMaps.end()) {
        ESP_LOGW(TAG, "Unknown section: %s", section.c_str());
        return;
    }

    auto &funcMap = sec->second;
    auto it = funcMap.find(name);

    // Check if function exists in map AND if the function object is valid
    if (it != funcMap.end()) { 
        if (it->second) { // This checks if the std::function is not null
            it->second(args);
        } else {
            ESP_LOGE(TAG, "Function [%s] exists but is uninitialized (null)", name.c_str());
        }
    } else {
        ESP_LOGW(TAG, "Unknown function [%s] in section [%s]", 
                 name.c_str(), section.c_str());
    }
}
