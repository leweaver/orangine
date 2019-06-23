#pragma once

namespace oe {

    enum class Material_type_index : uint8_t{
        PBR,
        Unlit,
        Deferred_Light,
        Skybox,
        Clear_G_Buffer,

        Num_Material_Type_Index
    };

    class IMaterial_repository {
	};

	class Material_repository : public IMaterial_repository {
	};
}
