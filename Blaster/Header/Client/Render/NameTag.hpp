#pragma once

#include <unordered_map>
#include <array>
#include <numbers>
#include <string>
#include <vector>
#include <cctype>
#include <glad/glad.h>
#include "Client/Render/ShaderManager.hpp"
#include "Client/Render/Vertices/FatVertex.hpp"
#include "Client/Network/ClientNetwork.hpp"
#include "Independent/ECS/GameObjectManager.hpp"
#include "Independent/Math/Transform.hpp"
#include "Independent/ComponentRegistry.hpp"
#include "Independent/ECS/ComponentFactory.hpp"

using namespace Blaster::Independent::Math;
using namespace Blaster::Independent::ECS;
using namespace Blaster::Client::Render::Vertices;
using namespace Blaster::Client::Network;

namespace Blaster::Client::Render
{
    class NameTag final : public Component
    {
    public:
        NameTag(const NameTag&) = delete;
        NameTag(NameTag&&) = delete;
        NameTag& operator=(const NameTag&) = delete;
        NameTag& operator=(NameTag&&) = delete;

        void Initialize() override
        {
            BuildGeometry();
            Generate();
            shader = ShaderManager::GetInstance().Get("blaster.fat").value();
        }

        void Update() override
        {
            const std::string path = "player-" + ClientNetwork::GetInstance().GetStringId() + ".camera";
            const auto optionalCameraGameObject = GameObjectManager::GetInstance().Get(path);
            if (!optionalCameraGameObject.has_value())
                return;
            const auto cameraTransform = optionalCameraGameObject.value()->GetTransform();
            const Vector<float, 3> direction = cameraTransform->GetWorldPosition() - GetGameObject()->GetTransform()->GetWorldPosition();
            const float yaw = std::atan2(direction.x(), direction.z()) * 180.0f / std::numbers::pi_v<float>;
            GetGameObject()->GetTransform()->SetLocalRotation({ 0.0f, yaw, 0.0f }, false);
        }

        void Render(const std::shared_ptr<Camera>& camera) override
        {
            if (GetGameObject()->IsLocallyControlled())
                return;
            shader->Bind();
            shader->SetUniform("projectionUniform", camera->GetProjectionMatrix());
            shader->SetUniform("viewUniform", camera->GetViewMatrix());
            shader->SetUniform("modelUniform", GetGameObject()->GetTransform()->GetModelMatrix());
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        static std::shared_ptr<NameTag> Create(const std::string& text)
        {
            std::shared_ptr<NameTag> result(new NameTag());
            result->text = text;
            return result;
        }

    private:
        NameTag() = default;
        ~NameTag() override
        {
            if (VAO)
                glDeleteVertexArrays(1, &VAO);
            if (VBO)
                glDeleteBuffers(1, &VBO);
            if (EBO)
                glDeleteBuffers(1, &EBO);
        }

        void Generate()
        {
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(FatVertex), vertices.data(), GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
            const auto layout = FatVertex::GetLayout();
            for (const auto& element : layout.GetElements())
            {
                glEnableVertexAttribArray(element.index);
                glVertexAttribPointer(element.index, element.componentCount, element.type, element.normalized, layout.GetStride(), reinterpret_cast<const void*>(element.offset));
            }
            glBindVertexArray(0);
        }

        void AddQuad(float x0, float y0, float x1, float y1)
        {
            const std::array<FatVertex, 4> quad = {
                FatVertex({ x0, y0, 0.0f },   { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }),
                FatVertex({ x1, y0, 0.0f },   { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }),
                FatVertex({ x1, y1, 0.0f },   { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }),
                FatVertex({ x0, y1, 0.0f },   { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f })
            };
            const uint32_t start = static_cast<uint32_t>(vertices.size());
            vertices.insert(vertices.end(), quad.begin(), quad.end());
            indices.push_back(start);
            indices.push_back(start + 1);
            indices.push_back(start + 2);
            indices.push_back(start);
            indices.push_back(start + 2);
            indices.push_back(start + 3);
        }

        void BuildGeometry()
        {
            static const std::unordered_map<char, std::array<std::string_view, 7>> fontMap = {
                { 'A',{ "01110","10001","10001","11111","10001","10001","10001" } },
                { 'E',{ "11111","10000","10000","11110","10000","10000","11111" } },
                { 'L',{ "10000","10000","10000","10000","10000","10000","11111" } },
                { 'P',{ "11110","10001","10001","11110","10000","10000","10000" } },
                { 'R',{ "11110","10001","10001","11110","10100","10010","10001" } },
                { 'Y',{ "10001","10001","01010","00100","00100","00100","00100" } },
                { '0',{ "01110","10001","10011","10101","11001","10001","01110" } },
                { '1',{ "00100","01100","00100","00100","00100","00100","01110" } },
                { '2',{ "01110","10001","00001","00110","01000","10000","11111" } },
                { '3',{ "11110","00001","00001","01110","00001","00001","11110" } },
                { '4',{ "00010","00110","01010","10010","11111","00010","00010" } },
                { '5',{ "11111","10000","11110","00001","00001","10001","01110" } },
                { '6',{ "01110","10000","10000","11110","10001","10001","01110" } },
                { '7',{ "11111","00001","00010","00100","01000","01000","01000" } },
                { '8',{ "01110","10001","10001","01110","10001","10001","01110" } },
                { '9',{ "01110","10001","10001","01111","00001","00001","01110" } }
            };
            const float pixel = 0.1f;
            float advance = 0.0f;
            for (char ch : text)
            {
                char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                auto it = fontMap.find(upper);
                if (it == fontMap.end())
                {
                    advance += 6 * pixel;
                    continue;
                }
                const auto& rows = it->second;
                for (int row = 0; row < 7; ++row)
                {
                    for (int col = 0; col < 5; ++col)
                    {
                        if (rows[row][col] == '1')
                        {
                            float x0 = advance + col * pixel;
                            float y0 = -static_cast<float>(row) * pixel;
                            float x1 = x0 + pixel;
                            float y1 = y0 - pixel;
                            AddQuad(x0, y0, x1, y1);
                        }
                    }
                }
                advance += 6 * pixel;
            }
            const float width = advance - pixel;
            const float height = 7 * pixel;
            for (auto& vertex : vertices)
            {
                vertex.position.x() -= width / 2.0f;
                vertex.position.y() += height / 2.0f;
            }
        }

        friend class boost::serialization::access;
        friend class Blaster::Independent::ECS::ComponentFactory;
        template <typename Archive>
        void serialize(Archive& archive, const unsigned)
        {
            archive & boost::serialization::base_object<Component>(*this);
            archive & boost::serialization::make_nvp("text", text);
        }

        std::string text;
        GLuint VAO = 0, VBO = 0, EBO = 0;
        std::vector<FatVertex> vertices;
        std::vector<uint32_t> indices;
        std::shared_ptr<Shader> shader;

        DESCRIBE_AND_REGISTER(NameTag, (Component), (), (), (text))
    };
}

REGISTER_COMPONENT(Blaster::Client::Render::NameTag)
