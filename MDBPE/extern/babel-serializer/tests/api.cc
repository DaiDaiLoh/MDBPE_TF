#include <nexus/test.hh>

#include <clean-core/vector.hh>

namespace babel::obj
{
template <class ScalarT>
struct mesh
{
    using scalar_t = ScalarT;

    // v x y z [w]
    struct pos
    {
        scalar_t x = 0;
        scalar_t y = 0;
        scalar_t z = 0;
        scalar_t w = 1;
    };

    // vt u [v] [w]
    struct texcoord
    {
        scalar_t u = 0;
        scalar_t v = 0;
        scalar_t w = 0;
    };

    // vn x y z
    struct normal
    {
        scalar_t x = 0;
        scalar_t y = 0;
        scalar_t z = 0;
    };

    // vp u [v] [w]
    struct parameter
    {
        scalar_t u = 0;
        scalar_t v = 0;
        scalar_t w = 0;
    };

    // f a b c ...
    struct face
    {
        /// start index in mesh::indices
        int start;
        /// 3 * count indices of mesh::indices are used
        int count;
    };

    // l a b c ..
    struct line
    {
        /// start index in mesh::indices
        int start;
        /// count indices of mesh::indices are used
        int count;
    };

    cc::vector<pos> positions;
    cc::vector<texcoord> texcoords;
    cc::vector<normal> normals;
    cc::vector<parameter> parameters;
    cc::vector<face> faces;
    cc::vector<line> lines;

    /// general purpose index vector
    /// used by faces and lines to reference into other vectors
    /// can contain -1 entries for "not used"
    /// (obj uses 1-based and supports relative indices but these are already translated in absolute 0-based here)
    cc::vector<int> indices;
};
}

// TEST("babel api") {}
