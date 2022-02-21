#include "OeCore/Texture.h"
#include "OeCore/EngineUtils.h"


using namespace oe;

void oe::to_json(nlohmann::json& j, const std::shared_ptr<Texture> texture) {
    // TODO: Return asset ID's here?
    j = nlohmann::json{ {"loaded", texture ? "yes" : "no" } };
}

void oe::from_json(const nlohmann::json& /*j*/, std::shared_ptr<Texture>& /*texture*/) {
    OE_THROW(std::runtime_error("Unsupported - texture deserialization"));
}
