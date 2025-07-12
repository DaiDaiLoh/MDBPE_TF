#include "MeshData.hh"

#include <glow/objects/ArrayBuffer.hh>
#include <glow/objects/ElementArrayBuffer.hh>
#include <glow/objects/VertexArray.hh>

glow::SharedVertexArray glow::assimp::MeshData::createVertexArray() const
{
    std::vector<SharedArrayBuffer> abs;

    if (!positions.empty())
    {
        auto ab = ArrayBuffer::create();
        ab->defineAttribute<tg::pos3>("aPosition");
        ab->bind().setData(positions);
        abs.push_back(ab);
    }
    if (!normals.empty())
    {
        auto ab = ArrayBuffer::create();
        ab->defineAttribute<tg::vec3>("aNormal");
        ab->bind().setData(normals);
        abs.push_back(ab);
    }
    if (!tangents.empty())
    {
        auto ab = ArrayBuffer::create();
        ab->defineAttribute<tg::vec3>("aTangent");
        ab->bind().setData(tangents);
        abs.push_back(ab);
    }
    for (auto i = 0u; i < colors.size(); ++i)
    {
        auto ab = ArrayBuffer::create();
        if (i == 0)
            ab->defineAttribute<tg::color4>("aColor");
        else
            ab->defineAttribute<tg::color4>("aColor" + std::to_string(i + 1));
        ab->bind().setData(colors[i]);
        abs.push_back(ab);
    }
    for (auto i = 0u; i < texCoords.size(); ++i)
    {
        auto ab = ArrayBuffer::create();
        if (i == 0)
            ab->defineAttribute<tg::pos2>("aTexCoord");
        else
            ab->defineAttribute<tg::pos2>("aTexCoord" + std::to_string(i + 1));
        ab->bind().setData(texCoords[i]);
        abs.push_back(ab);
    }

    for (auto const& ab : abs)
        ab->setObjectLabel(ab->getAttributes()[0].name + " of " + filename);

    auto eab = ElementArrayBuffer::create(indices);
    eab->setObjectLabel(filename);
    auto va = VertexArray::create(abs, eab, GL_TRIANGLES);
    va->setObjectLabel(filename);
    return va;
}
