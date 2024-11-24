#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp> // for pausing and resuming the game
#include <Geode/utils/web.hpp>
#include <Geode/loader/Event.hpp>
#include <Geode/utils/cocos.hpp> // for FLAlertLayer
#include <Geode/loader/Mod.hpp> // for getting config directory
#include <Geode/cocos/actions/CCActionManager.h> // include for action pausing
#include "json.hpp" // Include nlohmann::json for JSON parsing

#include <fstream> // for file reading and writing
#include <string> // for std::string
#include <random> // for random number generation

using json = nlohmann::json;
using namespace geode::prelude;

class $modify(MyPlayerObject, PlayerObject) {
    struct Fields {
        EventListener<web::WebTask> m_listener;
    };

    // Function to write the readme.txt file
    void writeReadme() {
        auto mod = Mod::get(); // Get the current mod instance
        auto configDir = mod->getConfigDir(true); // Get the mod's config directory

        std::ofstream readmeFile(configDir / "readme.txt");
        if (!readmeFile.is_open()) {
            log::error("Failed to create readme.txt in the config directory.");
            return;
        }

        readmeFile << R"(
=======================================================
        OpenShock Mod Configuration Documentation      
=======================================================

The `settings.json` file configures the OpenShock mod. 
This file must follow JSON format and include the necessary fields.

-------------------------------------------------------
Supported Fields
-------------------------------------------------------

╔══════════════╦════════╦════════╦══════════════╦═════════════════════════════════════╗
║ Field Name       ║ Type      ║ Required ║ Default Value    ║ Description                                    ║
╠══════════════╬════════╬════════╬══════════════╬═════════════════════════════════════╣
║ shockerID        ║ string    ║ Yes      ║ N/A              ║ Unique ID for the shocker device.              ║
║ OpenShockToken   ║ string    ║ Yes      ║ N/A              ║ API token for OpenShock service.               ║
║ minDuration      ║ integer   ║ No       ║ 300              ║ Minimum shock duration (ms). Must be >= 300.   ║
║ maxDuration      ║ integer   ║ No       ║ 30000            ║ Maximum shock duration (ms). Must be <= 30000. ║
║ minIntensity     ║ integer   ║ No       ║ 1                ║ Minimum shock intensity. Must be >= 1.         ║
║ maxIntensity     ║ integer   ║ No       ║ 100              ║ Maximum shock intensity. Must be <= 100.       ║
║ customName       ║ string    ║ Yes      ║ N/A              ║ Custom name for the shock control session.     ║
║ endpointDomain   ║ string    ║ No       ║ api.openshock.app║ API endpoint domain. Defaults if not provided. ║
╚══════════════╩════════╩════════╩══════════════╩═════════════════════════════════════╝

-------------------------------------------------------
Validation Rules
-------------------------------------------------------

1. **Duration Ranges**:
   - `minDuration` must be >= 300.
   - `maxDuration` must be <= 30000.
   - `minDuration` must not exceed `maxDuration`.

2. **Intensity Ranges**:
   - `minIntensity` must be >= 1.
   - `maxIntensity` must be <= 100.
   - `minIntensity` must not exceed `maxIntensity`.

3. **Required Fields**:
   - `shockerID`, `OpenShockToken`, and `customName` are mandatory.

4. **Endpoint Domain**:
   - If `endpointDomain` is missing or empty, defaults to `api.openshock.app`.

-------------------------------------------------------
Example Configuration File
-------------------------------------------------------

{
    "shockerID": "7a3e1c5b-fb7c-4b1c-8b6e-6a2e1f8b7d92",
    "OpenShockToken": "RXLOseP4PpBmE8w59JTHUFnrIEgd5hhgeGkACgvNz7vjadAbfMOiuTev824lYP0f",
    "minDuration": 500,
    "maxDuration": 10000,
    "minIntensity": 10,
    "maxIntensity": 90,
    "customName": "ShockControl",
    "endpointDomain": "api.customdomain.com"
}

-------------------------------------------------------
Default Behavior
-------------------------------------------------------

- If optional fields are omitted:
  - `minDuration`: Defaults to 300.
  - `maxDuration`: Defaults to 30000.
  - `minIntensity`: Defaults to 1.
  - `maxIntensity`: Defaults to 100.
  - `endpointDomain`: Defaults to `api.openshock.app`.

-------------------------------------------------------
Error Handling
-------------------------------------------------------

- Invalid configurations will cause the mod to malfunction.
- Errors are logged and displayed in-game via pop-ups.
- Required fields must not be empty.
- Ensure `endpointDomain` is valid if provided.

-------------------------------------------------------

This document provides all necessary details to configure the OpenShock mod correctly. For further assistance, consult the OpenShock API documentation or contact support.
)";
        readmeFile.close();
    }

    // Function to read and validate configuration values from the JSON file
    json readConfig() {
        auto mod = Mod::get(); // Get the current mod instance
        auto configDir = mod->getConfigDir(true); // Get the mod's config directory

        // Write the readme.txt file
        writeReadme();

        std::ifstream configFile(configDir / "settings.json");
        if (!configFile.is_open()) {
            log::error("Failed to open settings.json file in config directory");
            showPopupMessage("Error: Missing config file! Read readme.txt in the mod's config folder.");
            return {};
        }

        json configJson;
        try {
            configFile >> configJson; // Parse the JSON file
        } catch (const std::exception& e) {
            log::error("Error parsing JSON file: {}", e.what());
            showPopupMessage("Error: Invalid config file! Read readme.txt in the mod's config folder.");
            return {};
        }

        // Validate duration and intensity ranges
        int minDuration = configJson.value("minDuration", 300);
        int maxDuration = configJson.value("maxDuration", 30000);
        int minIntensity = configJson.value("minIntensity", 1);
        int maxIntensity = configJson.value("maxIntensity", 100);

        if (minDuration < 300 || maxDuration > 30000 || minDuration > maxDuration) {
            log::error("Invalid duration range in config: minDuration={}, maxDuration={}", minDuration, maxDuration);
            showPopupMessage("Error: Invalid config file! Read readme.txt in the mod's config folder.");
            return {};
        }

        if (minIntensity < 1 || maxIntensity > 100 || minIntensity > maxIntensity) {
            log::error("Invalid intensity range in config: minIntensity={}, maxIntensity={}", minIntensity, maxIntensity);
            showPopupMessage("Error: Invalid config file! Read readme.txt in the mod's config folder.");
            return {};
        }

        return configJson;
    }

    // Function to generate a random value within a range
    int generateRandomValue(int min, int max) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dist(min, max);
        return dist(gen);
    }

    // Hook into the player's death effect
    void playDeathEffect() {
        // Call the original death effect function to keep the default behavior
        PlayerObject::playDeathEffect();

        // Immediately pause the game and show "Shocking..."
        pauseGame();
        showPopupMessage("Shocking...");

        // Execute the custom web request after showing the message
        sendPostRequest();
    }

    // Function to send a POST request with JSON data
    void sendPostRequest() {
        // Read the configuration from the JSON file
        json config = readConfig();
        if (config.empty()) {
            return; // Exit if the configuration couldn't be read or is invalid
        }

        // Extract the min and max values for random generation
        int minDuration = config.value("minDuration", 300);
        int maxDuration = config.value("maxDuration", 30000);
        int minIntensity = config.value("minIntensity", 1);
        int maxIntensity = config.value("maxIntensity", 100);
        auto shockerID = config.value("shockerID", "");
        auto openShockToken = config.value("OpenShockToken", "");
        auto customName = config.value("customName", "");

        // Get the endpoint domain, default to api.openshock.app if missing or empty
        std::string endpointDomain = config.value("endpointDomain", "api.openshock.app");
        if (endpointDomain.empty()) {
            endpointDomain = "api.openshock.app";
        }

        if (shockerID.empty() || openShockToken.empty() || customName.empty()) {
            log::error("Missing required fields in JSON configuration");
            showPopupMessage("Error: Missing required fields in config file! Read readme.txt in the mod's config folder.");
            return;
        }

        // Generate random intensity and duration within the valid ranges
        int randomIntensity = generateRandomValue(minIntensity, maxIntensity);
        int randomDurationMs = generateRandomValue(minDuration, maxDuration);

        // Bind the listener to handle the response
        m_fields->m_listener.bind([this](web::WebTask::Event* e) {
            if (web::WebResponse* res = e->getValue()) {
                // Get the server response as a string
                std::string response = res->string().unwrapOr("No response from the server");

                // Show the response in a pop-up message
                showPopupMessage(response);

            } else if (web::WebProgress* p = e->getProgress()) {
                // Log the progress of the request if it's still in progress
                log::info("Request in progress... Download progress: {}%", p->downloadProgress().value_or(0.f) * 100);
            } else if (e->isCancelled()) {
                // Show a cancellation message in the pop-up
                showPopupMessage("Request was cancelled.");
            }
        });

        // Create the web request object
        auto req = web::WebRequest();

        // Set the request body as JSON (as a string), using the generated values
        json requestBody = {
            { "shocks", {{
                { "id", shockerID },
                { "type", "Shock" },
                { "intensity", randomIntensity },
                { "duration", randomDurationMs },
                { "exclusive", true }
            }}},
            { "customName", customName }
        };

        // Add the JSON body to the request
        req.bodyString(requestBody.dump());

        // Set the content type header to application/json
        req.header("Content-Type", "application/json");
        req.header("accept", "application/json");

        // Add the OpenShockToken header
        req.header("OpenShockToken", openShockToken);

        // Construct the full URL with the endpoint domain
        std::string url = "https://" + endpointDomain + "/2/shockers/control";
        m_fields->m_listener.setFilter(req.post(url));

        // Show the duration and intensity in a pop-up message
        showPopupMessage("Duration: " + std::to_string(randomDurationMs / 1000) + "s" + "     " + "Intensity: " + std::to_string(randomIntensity));
    }

    // Function to pause the game
    void pauseGame() {
        if (auto playLayer = PlayLayer::get()) {
            // Pause the PlayLayer
            playLayer->pauseGame(true); 

            // Pause all running actions in the game
            if (auto actionManager = cocos2d::CCDirector::sharedDirector()->getActionManager()) {
                actionManager->pauseAllRunningActions();
            }
        }
    }

    // Function to show a pop-up message
    void showPopupMessage(const std::string& message) {
        auto alertLayer = FLAlertLayer::create(nullptr, "Message", message.c_str(), "Continue", nullptr);
        alertLayer->onBtn1(this); // Set a tag for the pop-up layer
        alertLayer->show();
    }

};
