// BSP Single Header File
#pragma once
#include <ctime>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <set>
#include <span>
#include <unordered_map>
#include <vector>
#include <assert.h>
#include <cfloat>

#include "global.h"

inline std::vector<Model> models;

inline Shader default_shader;
inline Shader invisible_shader;
inline Shader debug_shader;

inline Shader liquid_shader;
inline int liquid_time_loc;

inline Shader sky_shader;
inline int sky_time_loc;
inline int sky_campos_loc;

inline float SLOPE_MAX = 0.4f;   // max slope angle we can walk up
inline float SLOPE_BOOST = 0.7f; // allows so we arent inching up slopes

/*
 TraceResult
 result of a single hull-trace through the BSP clipnode tree
 */
struct TraceResult
{
  float fraction = 1.0f;      // 0..1 — how far the move completed before impact
  Vector3 normal = {0, 0, 0}; // surface normal at impact, in raylib space
  bool started_solid = false; // origin was already inside solid
  bool all_solid = false;     // entire trace was inside solid
};

// -----------------------------------------------------------------------
// ReadT Helpers
// The _read<T> helper function uses these offsets to
// jump to the correct part of the file to pull out vertices, faces, or textures.
// -----------------------------------------------------------------------

/*
ReadT
Reads and returns entire data of an input stream
*/
template <typename T>
T ReadT(std::istream &stream)
{
  T data{};
  stream.read((char *)&data, sizeof(T));
  return data;
};

/*
ReadT
Reads and returns a specific position of data in an input stream
*/
template <typename T>
T ReadT(std::istream &stream, size_t idx)
{
  stream.seekg(idx * sizeof(T), std::ios::cur);
  return ReadT<T>(stream);
};

// -----------------------------------------------------------------------
// BSP Data Structures
// -----------------------------------------------------------------------

#pragma pack(push, 1) // pack the bytes as tight as possible to match the BSP/WAD/Texture data

/*
BSP files use Paletted Textures.
Instead of storing 24-bit Red/Green/Blue values for every pixel,
each pixel is a single uint8_t index (0-255).

The std::transform(..., palette) line converts those 8-bit indices into actual
Color_RGB8 (RGB) values using a texture palette.
*/
struct Color_RGB8
{
  uint8_t r, g, b;
}; // 8 bit color of RGB
Color_RGB8 palette(uint8_t id); // returns the texture palette

struct Vector3S
{
  int16_t x, y, z;
}; // 16 Bit Vector3 short (16bit int is known as a short)

struct BoundingBoxS
{
  Vector3S min, max;
}; // Bounding Box, Short values

/*
A Directory entry
Each Dir_Entry tells you the Offset (where the data starts in the file)
and the Size (how big the data section is).
*/
struct Dir_Entry
{
  int32_t offset; // Offset to entry, in bytes, from start of file
  int32_t size;   // Size of entry in file, in bytes
};

/*
The BSP file header
A BSP file has a Header at the very beginning that contains a "Table of Contents" (the Dir_Entry array).
*/
struct Header
{
  int32_t version; // Model version, must be 0x17 (23).

  Dir_Entry entities; // List of Entities.
  Dir_Entry planes;   // Map Planes.
  // numplanes = size/sizeof(plane_t)

  Dir_Entry miptex;   // Wall Textures.
  Dir_Entry vertices; // Map Vertices.
  // numvertices = size/sizeof(vertex_t)

  Dir_Entry visibility; // Leaves Visibility lists.
  Dir_Entry nodes;      // BSP Nodes.
  // numnodes = size/sizeof(node_t)

  Dir_Entry texinfos; // Texture Info for faces.
  // numtexinfo = size/sizeof(texinfo_t)

  Dir_Entry faces; // Faces of each surface.
  // numfaces = size/sizeof(face_t)

  Dir_Entry lightmaps; // Wall Light Maps.
  Dir_Entry clipnodes; // clip nodes, for Models.
  // numclips = size/sizeof(clipnode_t)

  Dir_Entry leaves; // BSP Leaves.
  // numleaves = size/sizeof(leaf_t)

  Dir_Entry listfaces; // List of Faces.
  Dir_Entry edges;     // Edges of faces.
  // numedges = size/sizeof(edge_t)

  Dir_Entry listedges; // List of Edges.
  Dir_Entry models;    // List of Models.
                       // nummodels = Size/sizeof(model_t)
};

/*
Entity
Key value pair stored in BSP
*/
struct Entity
{
  std::unordered_map<std::string, std::string> tags;
};

/*
BSP Model
BSP_Model: The "Model 0" is the map itself.
Other models in the list are "Sub-models" used for moving parts like elevators or doors.
*/
struct BSP_Model
{
  BoundingBox bound;    // The bounding box of the Model
  Vector3 origin;       // origin of model, usually (0,0,0)
  int32_t bsp_node_id;  // index of first BSP node
  int32_t clipnode1_id; // index of the first Clip node
  int32_t clipnode2_id; // index of the second Clip node
  int32_t _dummy_id;    // usually zero
  int32_t numleafs;     // number of BSP leaves
  int32_t face_id;      // index of Faces
  int32_t face_num;     // number of Faces
};

/*
Edge
Two indices pointing to the Vertex array. An edge is just a line between two points.
*/
struct Edge
{
  uint16_t vs; // index of the start vertex
               //  must be in [0,numvertices[
  uint16_t ve; // index of the end vertex
               //  must be in [0,numvertices[
};

/*
Plane
A mathematical plane (Ax+By+Cz=D).
Every wall in a BSP is technically an infinite plane that clips the world.
*/
struct Plane
{
  Vector3 normal; // Vector orthogonal to plane (Nx,Ny,Nz)
  float dist;     // Offset to plane, along the normal vector.
  int32_t type;   // Type of plane, depending on normal vector.
};

/*
TexInfo
This is the Texture Projection data.
It contains two vectors (U and V) used to calculate how a 2D image is draped over a 3D surface.
*/
struct TexInfo
{
  Vector3 u_axis;     // U vector, horizontal in texture space
  float u_offset;     // horizontal offset in texture space
  Vector3 v_axis;     // V vector, vertical in texture space
  float v_offset;     // vertical offset in texture space
  uint32_t miptex_id; // Index of Mip Texture
                      //   must be in [0,numtex[
  uint32_t animated;  // 0 for ordinary textures, 1 for water
};

/*
Face
A single polygon.
It links to a plane_id (direction), ledge_id (the edges forming the shape), and texinfo_id (the texture).
*/
struct Face
{
  uint16_t plane_id;   // The plane in which the face lies
                       //   must be in [0,numplanes[
  uint16_t side;       // 0 if in front of the plane, 1 if behind the plane
  int32_t ledge_id;    // first edge in the List of edges
                       //   must be in [0,numledges[
  uint16_t ledge_num;  // number of edges in the List of edges
  uint16_t texinfo_id; // index of the Texture info the face is part of
                       //   must be in [0,numtexinfos[
  uint8_t typelight;   // type of lighting, for the face
  uint8_t baselight;   // from 0xFF (dark) to 0 (bright)
  uint8_t light[2];    // two additional light models
  uint32_t lightmap;   // Pointer inside the general light map, or -1
                       //   this defines the start of the face light map
};

/*
Miptex
The header for a texture.
It contains the name, dimensions, and 4 offsets for mipmaps (lod levels).
*/
struct Miptex
{
  char name[16];      // Name of the texture.
  uint32_t width;     // width of picture, must be a multiple of 8
  uint32_t height;    // height of picture, must be a multiple of 8
  uint32_t offset[4]; // offsets to uint8_t Pix[width * height], relative to start of Miptex
};
/**
 * typedef struct                 // Mip texture list header
 * { int32_t numtex;                 // Number of textures in Mip Texture list
 *   int32_t offset[numtex];         // Offset to each of the individual texture from the beginning of mipheader_t
 * } mipheader_t;
 */

/*
Node
A "branch" in the tree.
It splits a volume of space into front and back children using a Plane.
*/
struct Node
{
  uint32_t plane_id; // The plane that splits the node
                     //   must be in [0,numplanes[
  int16_t front;     // If > 0,  front = index of Front child node
                     // else,   ~front = index of child leaf
  int16_t back;      // If > 0,   back = index of Back child node
                     // else,    ~back = index of child leaf
  BoundingBoxS box;  // Bounding box of node and all children
  uint16_t face_id;  // Index of first Polygons in the node
  uint16_t face_num; // Number of faces in the node
};

/*
Leaf
This represents an empty volume of space where the player can be.
It contains a list of Face indices that are visible from that spot.
*/
struct Leaf
{
  int32_t type;          // Special type of leaf
  int32_t visibility_id; // Beginning of visibility lists
                         //   must be -1 or in [0,numvislist[
  BoundingBoxS bound;    // Bounding box of the leaf
  uint16_t listface_id;  // First item of the list of faces
                         //   must be in [0,numlfaces[
  uint16_t listface_num; // Number of faces in the leaf
  uint8_t sndwater;      // level of the four ambient sounds:
  uint8_t sndsky;        //   0    is no sound
  uint8_t sndslime;      //   0xFF is maximum volume
  uint8_t sndlava;
};

// uint16_t listface[numlface];   // each uint16_t is the index of a Face
// int32_t listedge[numlstedge];
// uint8_t vislist[numvislist];    // RLE encoded bit array

/*
Clipnode
A simplified version of a Node used only for collision detection (physics), not rendering.
*/
struct Clipnode
{
  uint32_t planenum; // The plane which splits the node
  int16_t front;     // If positive, id of Front child node
                     // If -2, the Front part is inside the model
                     // If -1, the Front part is outside the model
  int16_t back;      // If positive, id of Back child node
                     // If -2, the Back part is inside the model
                     // If -1, the Back part is outside the model
};

// uint8_t lightmap[numlightmap]; // value 0:dark 255:bright
// uint8_t light[width*height];

#pragma pack(pop)

// -----------------------------------------------------------------------
// BSP File Parsing
// -----------------------------------------------------------------------

/*
ReadEntity
Entities (player starts, lights, monsters) are stored as text strings in curly braces { "classname" "info_player_start" }.
This function uses std::quoted to extract the key-value pairs into an unordered_map.
*/
inline Entity ReadEntity(std::istream &stream)
{
  Entity entity{};

  char token;
  stream >> token;
  if (token != '{')
    throw TextFormat("Expected ')', found %c", token);

  while (stream >> std::ws)
  {
    token = stream.peek();
    if (token == '"')
    {
      std::string tag, tagValue;
      stream >> std::quoted(tag) >> std::quoted(tagValue);
      entity.tags[tag] = tagValue;
    }
    else if (token == '}')
    {
      stream >> token;
      break;
    }
    else
      throw TextFormat("Expected ')', found %c", token);
  }

  return entity;
};

/*
FromQuake
Converts .bsp xyz coords to raylib coords
*/
inline float bsp_raylib_scale = 0.05f;
inline Vector3 FromQuake(Vector3 quakeVec)
{
  return Vector3Scale({quakeVec.y, quakeVec.z, quakeVec.x}, bsp_raylib_scale);
};
inline Vector3 ToQuake(Vector3 rl)
{
  return {rl.z / bsp_raylib_scale, rl.x / bsp_raylib_scale, rl.y / bsp_raylib_scale};
};

/*
BSP_File
Wraps the raw BSP file stream and provides typed accessors for every data lump.
*/
static inline std::vector<uint8_t> lightmap_lump;
struct BSP_File
{
  std::istream &bsp_file;
  Header header;

  BSP_File(std::ifstream &_file) : bsp_file(_file)
  {
    if (bsp_file.good() == false)
      throw std::runtime_error("Failed to open file");

    header = ReadT<Header>(bsp_file);

    bsp_file.seekg(header.lightmaps.offset);
    lightmap_lump.resize(header.lightmaps.size);
    bsp_file.read((char *)lightmap_lump.data(), header.lightmaps.size);
  }

  template <typename T>
  T _read(Dir_Entry dir, size_t idx)
  {
    assert(idx < dir.size / sizeof(T));
    bsp_file.clear();
    bsp_file.seekg(dir.offset);
    return ReadT<T>(bsp_file, idx);
  }

  std::vector<Entity> entities()
  {
    bsp_file.clear();
    bsp_file.seekg(header.entities.offset);

    std::vector<Entity> entities{};
    while (bsp_file.tellg() < header.entities.offset + header.entities.size)
    {
      bsp_file >> std::ws; // skip whitespace
      int next = bsp_file.peek();
      if (next == EOF || next != '{')
        break;
      entities.push_back(ReadEntity(bsp_file));
    }

    return entities;
  }

  Plane plane(size_t idx) { return _read<Plane>(header.planes, idx); }

  int32_t miptex_count()
  {
    bsp_file.clear();
    bsp_file.seekg(header.miptex.offset);
    return ReadT<int32_t>(bsp_file);
  }

  Miptex miptex(size_t idx)
  {
    int32_t count = miptex_count();
    int32_t offset = ReadT<int32_t>(bsp_file, idx);
    bsp_file.seekg(header.miptex.offset + offset);
    return ReadT<Miptex>(bsp_file);
  }

  Vector3 vertex(size_t idx) { return _read<Vector3>(header.vertices, idx); }
  Node node(size_t idx) { return _read<Node>(header.nodes, idx); }
  TexInfo texinfo(size_t idx) { return _read<TexInfo>(header.texinfos, idx); }
  Face face(size_t idx) { return _read<Face>(header.faces, idx); }
  Leaf leaf(size_t idx) { return _read<Leaf>(header.leaves, idx); }
  uint16_t listface(size_t idx) { return _read<uint16_t>(header.listfaces, idx); }
  Edge edge(size_t idx) { return _read<Edge>(header.edges, idx); }
  int32_t listedge(size_t idx) { return _read<int32_t>(header.listedges, idx); }
  BSP_Model model(size_t idx) { return _read<BSP_Model>(header.models, idx); }

  std::vector<Color_RGB8> miptex_data(size_t idx, uint8_t miplevel)
  {
    Miptex mptx = miptex(idx);

    uint32_t width = mptx.width >> miplevel;
    uint32_t height = mptx.height >> miplevel;

    bsp_file.seekg(-sizeof(Miptex) + mptx.offset[miplevel], std::ios::cur);

    std::vector<uint8_t> palette_indices(width * height);
    bsp_file.read((char *)palette_indices.data(), width * height);

    std::vector<Color_RGB8> color_data;
    std::transform(palette_indices.begin(), palette_indices.end(), std::back_inserter(color_data), palette);
    return color_data;
  }

  // -----------------------------------------------------------------------
  // Lightmap
  // -----------------------------------------------------------------------

  /*
  FaceLightmapExtents
  The lightmap for a face is a small greyscale patch stored in the lightmap lump.
  Its size is determined by projecting all the face's vertices through the same
  u/v axes used for texturing, snapping to a 16-unit grid, then measuring the span.
  */
  struct FaceLightmapExtents
  {
    int lm_mins_s; // leftmost lightmap texel column (in texel space)
    int lm_mins_t; // topmost  lightmap texel row
    int width;     // patch width  in texels
    int height;    // patch height in texels
  };

  /*
  face_lightmap_extents
  Computes the lightmap patch dimensions for a single face.
  */
  inline FaceLightmapExtents face_lightmap_extents(const Face &face)
  {
    TexInfo info = texinfo(face.texinfo_id);
    float min_s = FLT_MAX, max_s = -FLT_MAX;
    float min_t = FLT_MAX, max_t = -FLT_MAX;

    for (size_t i = 0; i < face.ledge_num; i++)
    {
      int32_t le = listedge(face.ledge_id + i);
      Edge e = edge(std::abs(le));
      Vector3 v = vertex(le >= 0 ? e.vs : e.ve);

      float s = Vector3DotProduct(v, info.u_axis) + info.u_offset;
      float t = Vector3DotProduct(v, info.v_axis) + info.v_offset;
      min_s = std::min(min_s, s);
      max_s = std::max(max_s, s);
      min_t = std::min(min_t, t);
      max_t = std::max(max_t, t);
    }

    int lm_mins_s = (int)std::floor(min_s / 16.0f);
    int lm_mins_t = (int)std::floor(min_t / 16.0f);
    int lm_maxs_s = (int)std::ceil(max_s / 16.0f);
    int lm_maxs_t = (int)std::ceil(max_t / 16.0f);

    return {lm_mins_s, lm_mins_t,
            lm_maxs_s - lm_mins_s + 1,
            lm_maxs_t - lm_mins_t + 1};
  }

  /*
  face_lightmap_bytes
  Reads the raw 8-bit greyscale lightmap patch for a face out of the lightmap lump.
  Returns a full-bright patch if the face has no lightmap data (sky, triggers, etc.).
  */
  inline std::vector<uint8_t> face_lightmap_bytes(const Face &face, const FaceLightmapExtents &ext)
  {
    int count = ext.width * ext.height;
    if (face.lightmap == 0xFFFFFFFF || header.lightmaps.size == 0 || count <= 0)
      return std::vector<uint8_t>(count, 255);

    auto begin = lightmap_lump.begin() + face.lightmap;
    return std::vector<uint8_t>(begin, begin + count);
  }
};

// -----------------------------------------------------------------------
// Mesh Generation
// -----------------------------------------------------------------------

/*
VerticesNormal
It takes three points, creates two vectors, and performs a Cross Product to find the perpendicular "Normal" vector.
*/
static inline Vector3 VerticesNormal(Vector3 a, Vector3 b, Vector3 c)
{
  Vector3 ba = Vector3Subtract(b, a);
  Vector3 ca = Vector3Subtract(c, a);
  return Vector3Normalize(Vector3CrossProduct(ba, ca));
};

/*
GenMeshFaces
Generates the faces for each brush/polygon in the BSP
*/
static inline Mesh GenMeshFaces(BSP_File &map, std::span<const Face> faces)
{
  static_assert(sizeof(Vector3) == 3 * sizeof(float));
  static_assert(sizeof(Vector2) == 2 * sizeof(float));
  std::vector<Vector3> vertices{};
  std::vector<Vector2> texcoords{};
  std::vector<Vector3> normals{};
  std::vector<unsigned char> colors{}; // RGBA vertex colors, baked from lightmap

  for (const Face &face : faces)
  {
    TexInfo texinfo = map.texinfo(face.texinfo_id);
    Miptex miptex = map.miptex(texinfo.miptex_id);

    std::vector<Vector3> face_vertices{};
    std::vector<Vector2> face_texcoords{};
    std::vector<Vector2> face_raw_st{}; // raw s,t before dividing by texture size

    for (size_t i = 0; i < face.ledge_num; ++i)
    {
      int16_t ledge = map.listedge(face.ledge_id + i);
      Edge edge = map.edge(labs(ledge));
      Vector3 vertex = map.vertex(ledge >= 0 ? edge.vs : edge.ve);
      face_vertices.push_back(vertex);

      float s = Vector3DotProduct(vertex, texinfo.u_axis) + texinfo.u_offset;
      float t = Vector3DotProduct(vertex, texinfo.v_axis) + texinfo.v_offset;
      face_raw_st.push_back({s, t});

      face_texcoords.push_back({s / (float)miptex.width,
                                t / (float)miptex.height});
    }
    assert(face_vertices.empty() == false);

    // ---- lightmap for this face ----------------------------------------
    BSP_File::FaceLightmapExtents lm_ext = map.face_lightmap_extents(face);
    std::vector<uint8_t> lm_bytes = map.face_lightmap_bytes(face, lm_ext);

    // bilinear sample of the greyscale lightmap patch
    auto sample_lm = [&](float ls, float lt) -> uint8_t
    {
      if (lm_bytes.empty())
        return 255;
      ls = std::max(0.0f, std::min((float)(lm_ext.width - 1), ls));
      lt = std::max(0.0f, std::min((float)(lm_ext.height - 1), lt));
      int x0 = (int)ls, y0 = (int)lt;
      int x1 = std::min(x0 + 1, lm_ext.width - 1);
      int y1 = std::min(y0 + 1, lm_ext.height - 1);
      float fx = ls - x0, fy = lt - y0;
      float v = lm_bytes[y0 * lm_ext.width + x0] * (1 - fx) * (1 - fy) + lm_bytes[y0 * lm_ext.width + x1] * fx * (1 - fy) + lm_bytes[y1 * lm_ext.width + x0] * (1 - fx) * fy + lm_bytes[y1 * lm_ext.width + x1] * fx * fy;
      return (uint8_t)std::min(255.0f, v);
    };
    // --------------------------------------------------------------------

    for (size_t i = face_vertices.size() - 2; i > 0; --i)
    {
      size_t i0 = face_vertices.size() - 1; // .back()
      size_t i1 = i;
      size_t i2 = i - 1;

      Vector3 v[3] = {
          FromQuake(face_vertices[i0]),
          FromQuake(face_vertices[i1]),
          FromQuake(face_vertices[i2]),
      };
      vertices.insert(vertices.end(), std::begin(v), std::end(v));

      texcoords.push_back(face_texcoords[i0]);
      texcoords.push_back(face_texcoords[i1]);
      texcoords.push_back(face_texcoords[i2]);

      // sample lightmap brightness at each vertex and store as RGBA color
      for (size_t j : {i0, i1, i2})
      {
        float ls = face_raw_st[j].x / 16.0f - (float)lm_ext.lm_mins_s;
        float lt = face_raw_st[j].y / 16.0f - (float)lm_ext.lm_mins_t;
        uint8_t b = sample_lm(ls, lt);
        colors.push_back(b);   // R
        colors.push_back(b);   // G
        colors.push_back(b);   // B
        colors.push_back(255); // A
      }

      Vector3 normal = VerticesNormal(v[0], v[1], v[2]);
      for (size_t k = 0; k < 3; ++k)
        normals.push_back(normal);
    }
  }

  Mesh mesh{};
  mesh.vertexCount = vertices.size();
  mesh.vertices = (float *)vertices.data();
  mesh.texcoords = (float *)texcoords.data();
  mesh.normals = (float *)normals.data();
  mesh.colors = colors.data();
  UploadMesh(&mesh, false);

  // So the free functions don't complain later on
  mesh.vertices = (float *)malloc(1);
  mesh.texcoords = (float *)malloc(1);
  mesh.normals = (float *)malloc(1);
  mesh.colors = (unsigned char *)malloc(4);
  return mesh;
};

// -----------------------------------------------------------------------
// PVS (Potentially Visible Set) System
// -----------------------------------------------------------------------

/*
PVS
tells us which clusters are visible from any given cluster
*/
struct PVS
{
  std::vector<uint8_t> compressed_data;
  int num_leaves = 0;

  std::vector<bool> DecompressForLeaf(int vis_offset) const
  {
    std::vector<bool> visible(num_leaves, false);

    if (vis_offset < 0 || vis_offset >= (int)compressed_data.size())
      return std::vector<bool>(num_leaves, true); // no data = show all

    const uint8_t *ptr = compressed_data.data() + vis_offset;
    const uint8_t *end = compressed_data.data() + compressed_data.size();

    int leaf = 1; // leaf 0 is always the solid leaf, skip it
    while (leaf < num_leaves)
    {
      if (ptr >= end)
        break;
      uint8_t byte = *ptr++;

      if (byte == 0)
      {
        if (ptr >= end)
          break;
        leaf += (*ptr++) * 8;
      }
      else
      {
        for (int bit = 0; bit < 8 && leaf < num_leaves; bit++, leaf++)
          if (byte & (1 << bit))
            visible[leaf] = true;
      }
    }
    return visible;
  }
};

// -----------------------------------------------------------------------
// BSP Renderer
// -----------------------------------------------------------------------

struct BSP_Renderer
{
  // vertex range in texturebatch vbo for a cluster
  struct VisRange
  {
    int offset;
    int count;
  };

  // One GPU mesh per unique texture.
  // cluster_ranges maps vis_key (vertex_offset, vertex_count) within that mesh
  // rlDrawVertexArray(range.offset, range.count) draws only the visible portion.
  struct TextureBatch
  {
    Mesh mesh = {}; // used for sub-range draws
    Texture2D texture = {};
    Shader shader = {};
    int tex_loc = -1;                                 // cached texture sampler uniform location
    std::unordered_map<int, VisRange> cluster_ranges; // vis_key -> vertex range
  };

  // BSP data
  std::unique_ptr<std::ifstream> file_stream;
  std::unique_ptr<BSP_File> bsp_file;
  std::vector<Model> all_models; // fallback (unused with texture batches)
  std::vector<TextureBatch> texture_batches;
  int bsp_root_node = 0;
  int cluster_count = 0;
  bool has_pvs = true;

  // texture filter
  int texture_filter = 0;

  // pvs debug
  int last_draw_count = 0;
  int total_model_count = 0;

  // pvs data
  PVS pvs;
  std::vector<Node> all_nodes;
  std::vector<Leaf> all_leaves;
  std::unordered_map<int, int> leaf_to_cluster;

  // face and texture data
  std::unordered_map<std::string, std::vector<Face>> faces_by_texture;
  std::unordered_map<std::string, Texture> textures;
  std::unordered_map<int, std::string> face_to_texture;

  // face indices that belong to brush entity submodels (func_door, trigger_*, etc.)
  // populated before BuildClusterModels so those faces are excluded from static batches
  std::set<int32_t> submodel_face_ids;

  /*
BuildClusterModels
builds sections (clusters) of models seperated up for better lookup when culling
*/
  void BuildClusterModels()
  {
    default_shader = LoadShader(VS_PATH, FS_PATH);
    invisible_shader = LoadShader("gamedata/shaders/330/invisible.vs", "gamedata/shaders/330/invisible.fs");
    liquid_shader = LoadShader("gamedata/shaders/330/water.vs", "gamedata/shaders/330/water.fs");
    debug_shader = LoadShader("gamedata/shaders/330/debug.vs", "gamedata/shaders/330/debug.fs");

    liquid_time_loc = GetShaderLocation(liquid_shader, "time");
    sky_shader = LoadShader("gamedata/shaders/330/sky.vs", "gamedata/shaders/330/sky.fs");
    sky_time_loc = GetShaderLocation(sky_shader, "time");
    sky_campos_loc = GetShaderLocation(sky_shader, "cameraPos");

    // group faces
    std::unordered_map<std::string, std::unordered_map<int, std::vector<Face>>> by_tex_cluster;
    std::unordered_map<int, std::set<int>> processed;

    for (int leaf_id = 1; leaf_id < (int)all_leaves.size(); leaf_id++)
    {
      Leaf &leaf = all_leaves[leaf_id];
      int vis_key = leaf.visibility_id;

      for (int i = 0; i < (int)leaf.listface_num; i++)
      {
        uint16_t face_id = bsp_file->listface(leaf.listface_id + i);
        if (processed[vis_key].count(face_id))
          continue;
        if (submodel_face_ids.count(face_id))
          continue; // belongs to a brush entity — handled separately
        processed[vis_key].insert(face_id);

        Face face = bsp_file->face(face_id);
        TexInfo texinfo = bsp_file->texinfo(face.texinfo_id);
        Miptex miptex = bsp_file->miptex(texinfo.miptex_id);
        by_tex_cluster[std::string(miptex.name)][vis_key].push_back(face);
      }
    }

    // adds bsp face's triangles into arrays of verts
    auto append_face = [&](
                           const Face &face,
                           std::vector<Vector3> &verts,
                           std::vector<Vector2> &tcs,
                           std::vector<Vector3> &norms,
                           std::vector<uint8_t> &cols)
    {
      TexInfo texinfo = bsp_file->texinfo(face.texinfo_id);
      Miptex miptex = bsp_file->miptex(texinfo.miptex_id);

      std::vector<Vector3> fv;
      std::vector<Vector2> ftc, fst;

      for (size_t i = 0; i < face.ledge_num; ++i)
      {
        int16_t ledge = bsp_file->listedge(face.ledge_id + i);
        Edge edge = bsp_file->edge(labs(ledge));
        Vector3 vertex = bsp_file->vertex(ledge >= 0 ? edge.vs : edge.ve);
        fv.push_back(vertex);
        float s = Vector3DotProduct(vertex, texinfo.u_axis) + texinfo.u_offset;
        float t = Vector3DotProduct(vertex, texinfo.v_axis) + texinfo.v_offset;
        fst.push_back({s, t});
        ftc.push_back({s / (float)miptex.width, t / (float)miptex.height});
      }
      if (fv.empty())
        return;

      BSP_File::FaceLightmapExtents lm_ext = bsp_file->face_lightmap_extents(face);
      std::vector<uint8_t> lm_bytes = bsp_file->face_lightmap_bytes(face, lm_ext);

      auto sample_lm = [&](float ls, float lt) -> uint8_t
      {
        if (lm_bytes.empty())
          return 255;
        ls = std::max(0.0f, std::min((float)(lm_ext.width - 1), ls));
        lt = std::max(0.0f, std::min((float)(lm_ext.height - 1), lt));
        int x0 = (int)ls, y0 = (int)lt;
        int x1 = std::min(x0 + 1, lm_ext.width - 1);
        int y1 = std::min(y0 + 1, lm_ext.height - 1);
        float fx = ls - x0, fy = lt - y0;
        float v = lm_bytes[y0 * lm_ext.width + x0] * (1 - fx) * (1 - fy) + lm_bytes[y0 * lm_ext.width + x1] * fx * (1 - fy) + lm_bytes[y1 * lm_ext.width + x0] * (1 - fx) * fy + lm_bytes[y1 * lm_ext.width + x1] * fx * fy;
        return (uint8_t)std::min(255.0f, v);
      };

      for (size_t i = fv.size() - 2; i > 0; --i)
      {
        size_t i0 = fv.size() - 1, i1 = i, i2 = i - 1;
        Vector3 v[3] = {FromQuake(fv[i0]), FromQuake(fv[i1]), FromQuake(fv[i2])};
        verts.insert(verts.end(), std::begin(v), std::end(v));
        tcs.push_back(ftc[i0]);
        tcs.push_back(ftc[i1]);
        tcs.push_back(ftc[i2]);
        for (size_t j : {i0, i1, i2})
        {
          float ls = fst[j].x / 16.0f - (float)lm_ext.lm_mins_s;
          float lt = fst[j].y / 16.0f - (float)lm_ext.lm_mins_t;
          uint8_t b = sample_lm(ls, lt);
          cols.push_back(b);
          cols.push_back(b);
          cols.push_back(b);
          cols.push_back(255);
        }
        Vector3 n = VerticesNormal(v[0], v[1], v[2]);
        norms.push_back(n);
        norms.push_back(n);
        norms.push_back(n);
      }
    };

    // build texture batch per texture
    for (auto &[texname, cluster_face_map] : by_tex_cluster)
    {
      std::vector<Vector3> verts;
      std::vector<Vector2> tcs;
      std::vector<Vector3> norms;
      std::vector<uint8_t> cols;

      TextureBatch batch;

      // cluster order
      std::vector<int> sorted_keys;
      for (auto &[k, _] : cluster_face_map)
        sorted_keys.push_back(k);
      std::sort(sorted_keys.begin(), sorted_keys.end());

      for (int vis_key : sorted_keys)
      {
        int range_start = (int)verts.size();
        for (const Face &face : cluster_face_map[vis_key])
          append_face(face, verts, tcs, norms, cols);
        int range_count = (int)verts.size() - range_start;
        if (range_count > 0)
          batch.cluster_ranges[vis_key] = {range_start, range_count};
      }

      if (verts.empty())
        continue;

      // only 1 gpu upload for entire geometry baby
      Mesh mesh{};
      mesh.vertexCount = (int)verts.size();
      mesh.vertices = (float *)verts.data();
      mesh.texcoords = (float *)tcs.data();
      mesh.normals = (float *)norms.data();
      mesh.colors = cols.data();
      UploadMesh(&mesh, false);

      // replace with dummies so unloadmesh doesnt double free
      mesh.vertices = (float *)malloc(1);
      mesh.texcoords = (float *)malloc(1);
      mesh.normals = (float *)malloc(1);
      mesh.colors = (unsigned char *)malloc(4);
      batch.mesh = mesh;

      // Texture
      auto tex_it = textures.find(texname);
      if (tex_it != textures.end())
        batch.texture = tex_it->second;

      // Shader
      bool is_liquid = texname.at(0) == '*';
      bool is_sky = texname.starts_with("sky");
      bool is_invisible = texname.starts_with("clip") || texname.starts_with("skip") || texname.starts_with("trigger");

      if (is_liquid)
        batch.shader = liquid_shader;
      else if (is_sky)
      {
        batch.shader = sky_shader;
        SetTextureWrap(batch.texture, TEXTURE_WRAP_REPEAT);
        SetTextureFilter(batch.texture, TEXTURE_FILTER_BILINEAR);
      }
      else if (is_invisible)
        batch.shader = invisible_shader;
      else
        batch.shader = default_shader;

      // cache texture sampler location (sky uses "tex", others use "texture0")
      batch.tex_loc = batch.shader.locs[SHADER_LOC_MAP_DIFFUSE];
      if (batch.tex_loc == -1)
        batch.tex_loc = GetShaderLocation(batch.shader, "tex");

      texture_batches.push_back(std::move(batch));
    }

    // total possible sub-range draws (cluster ranges across all texture batches)
    total_model_count = 0;
    for (auto &batch : texture_batches)
      total_model_count += (int)batch.cluster_ranges.size();
  };

  /*
  LoadFromFile
  load bsp from file path :)
  */
  void LoadFromFile(const std::filesystem::path &path)
  {
    file_stream = std::make_unique<std::ifstream>(path, std::ios::binary);
    bsp_file = std::make_unique<BSP_File>(*file_stream);

    // load all nodes & leaves
    bsp_root_node = bsp_file->model(0).bsp_node_id;

    int num_nodes = bsp_file->header.nodes.size / sizeof(Node);
    all_nodes.reserve(num_nodes);
    for (int i = 0; i < num_nodes; i++)
      all_nodes.push_back(bsp_file->node(i));

    int num_leaves = bsp_file->header.leaves.size / sizeof(Leaf);
    all_leaves.reserve(num_leaves);
    for (int i = 0; i < num_leaves; i++)
      all_leaves.push_back(bsp_file->leaf(i));

    // load pvs data
    if (bsp_file->header.visibility.size > 0)
    {
      has_pvs = true;

      // Q1: vis lump is raw RLE bytes, no header
      bsp_file->bsp_file.clear();
      bsp_file->bsp_file.seekg(bsp_file->header.visibility.offset);
      pvs.compressed_data.resize(bsp_file->header.visibility.size);
      bsp_file->bsp_file.read((char *)pvs.compressed_data.data(),
                              bsp_file->header.visibility.size);

      // in Q1, each leaf IS its own cluster
      cluster_count = bsp_file->header.leaves.size / sizeof(Leaf);
      pvs.num_leaves = cluster_count;

      BuildLeafToClusterMapping();
    }

    // collect face indices that belong to brush entity submodels (model index > 0)
    // so BuildClusterModels can exclude them from the static world batches
    int num_models = bsp_file->header.models.size / sizeof(BSP_Model);
    for (int m = 1; m < num_models; m++)
    {
      BSP_Model submodel = bsp_file->model(m);
      for (int f = 0; f < submodel.face_num; f++)
        submodel_face_ids.insert(submodel.face_id + f);
    }

    // build face to texture mapping
    BuildFaceMappings();
    BuildClusterModels();
  }

  /*
  BuildLeafToClusterMapping
  figures out which cluster each leaf belongs to
  clusters are groups of leaves for culling
  */
  void BuildLeafToClusterMapping()
  {
    for (int i = 0; i < (int)all_leaves.size(); i++)
      leaf_to_cluster[i] = all_leaves[i].visibility_id;
  }

  /*
  FindClusterForVisOffset
  searches through all clusters to find which one has this visibility offset
  */
  int FindClusterForVisOffset(int vis_offset)
  {
    // find which cluster has this PVS offset
    for (int c = 0; c < cluster_count; c++)
    {
      if (c * 4 + 4 <= (int)pvs.compressed_data.size())
      {
        int32_t offset = *(int32_t *)(pvs.compressed_data.data() + c * 4);
        if (offset == vis_offset)
          return c;
      }
    }
    return -1;
  };

  /*
  BuildFaceMappings
  collects all faces from all leaves and groups them by texture
  also loads all the textures
  */
  void BuildFaceMappings()
  {
    // get faces from leaves
    std::set<int> processed_faces;

    for (int leaf_id = 0; leaf_id < (int)all_leaves.size(); leaf_id++)
    {
      Leaf &leaf = all_leaves[leaf_id];

      for (int i = 0; i < (int)leaf.listface_num; i++)
      {
        uint16_t face_id = bsp_file->listface(leaf.listface_id + i);

        if (processed_faces.count(face_id) > 0)
          continue;

        processed_faces.insert(face_id);

        Face face = bsp_file->face(face_id);
        TexInfo texinfo = bsp_file->texinfo(face.texinfo_id);
        Miptex miptex = bsp_file->miptex(texinfo.miptex_id);
        std::string texname = miptex.name;

        // remove null terminator padding
        texname = std::string(texname.c_str());

        faces_by_texture[texname].push_back(face);
        face_to_texture[face_id] = texname;

        // load texture
        if (textures.find(texname) == textures.end())
        {
          std::vector<Color_RGB8> color_data = bsp_file->miptex_data(texinfo.miptex_id, 0);
          Image texture_image = {
              .data = color_data.data(),
              .width = (int)miptex.width,
              .height = (int)miptex.height,
              .mipmaps = 1,
              .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8,
          };
          textures[texname] = LoadTextureFromImage(texture_image);
          GenTextureMipmaps(&textures[texname]);
          SetTextureFilter(textures[texname], texture_filter);
        }
      }
    }

    // build models for all faces (for fallback rendering)
    // for (auto &[texname, faces] : faces_by_texture)
    // {
    //   Mesh mesh = GenMeshFaces(*bsp_file, faces);
    //   Model model = LoadModelFromMesh(mesh);

    //   model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textures[texname];
    //   all_models.push_back(model);
    // }
  };

  /*
  FindLeaf
  searches the bsp tree to find which leaf contains the camera position
  returns the leaf index
  */
  int FindLeaf(Vector3 raylib_pos)
  {
    Vector3 quake_pos = ToQuake(raylib_pos);
    int node_id = bsp_root_node;

    while (node_id >= 0)
    {
      Node &node = all_nodes[node_id];
      Plane plane = bsp_file->plane(node.plane_id);

      float dist = Vector3DotProduct(plane.normal, quake_pos) - plane.dist;

      if (dist >= 0)
        node_id = node.front;
      else
        node_id = node.back;
    }

    // node ID is encoded as ~(leaf_index) when its a leaf
    return ~node_id;
  };

  /*
  GetVisibleLeaves
  returns all leaves that are potentially visible from the current leaf
  uses pvs data to skip leaves we cant see
  */
  std::set<int> GetVisibleLeaves(int current_leaf)
  {
    std::set<int> visible_vis_keys;

    // fallback - show all
    auto show_all = [&]()
    {
      for (auto &batch : texture_batches)
        for (auto &[vis_key, _] : batch.cluster_ranges)
          visible_vis_keys.insert(vis_key);
    };

    if (current_leaf < 0 || current_leaf >= (int)all_leaves.size())
    {
      show_all();
      return visible_vis_keys;
    }

    int vis_offset = all_leaves[current_leaf].visibility_id;
    visible_vis_keys.insert(vis_offset); // always include current leaf's cluster

    if (!has_pvs || vis_offset < 0)
    {
      show_all();
      return visible_vis_keys;
    }

    std::vector<bool> visible = pvs.DecompressForLeaf(vis_offset);

    // collect vis_keys of all visible leaves
    for (int i = 1; i < (int)all_leaves.size(); i++)
      if (i < (int)visible.size() && visible[i])
        visible_vis_keys.insert(all_leaves[i].visibility_id);

    // always include clusters with no PVS data (vis_id < 0)
    // these are non-solid leaves (triggers, water, etc.) that should always be drawn
    for (auto &batch : texture_batches)
      for (auto &[vis_key, _] : batch.cluster_ranges)
        if (vis_key < 0)
          visible_vis_keys.insert(vis_key);

    return visible_vis_keys;
  };

  /*
  DrawWithPVS
  draw all geometry in a bsp file with pvs culling
  */
  void DrawWithPVS(Shader &shader, Vector3 camera_pos, bool enable_wireframe)
  {
    last_draw_count = 0;
    float current_time = (float)GetTime();
    SetShaderValue(liquid_shader, liquid_time_loc, &current_time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sky_shader, sky_time_loc, &current_time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(sky_shader, sky_campos_loc, &camera_pos, SHADER_UNIFORM_VEC3);

    int current_leaf = FindLeaf(camera_pos);
    std::set<int> visible_vis_keys = GetVisibleLeaves(current_leaf);

    // MVP is constant for all batches this frame (geometry is already in world space)
    Matrix matMVP = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    int slot = 0;

    for (auto &batch : texture_batches)
    {
      // Bind shader and set per-frame uniforms
      rlEnableShader(batch.shader.id);

      if (batch.shader.locs[SHADER_LOC_MATRIX_MVP] != -1)
        rlSetUniformMatrix(batch.shader.locs[SHADER_LOC_MATRIX_MVP], matMVP);

      // matModel needed by sky.vs to compute world-space fragPosition.
      // BSP geometry is already in world space so model = identity.
      if (batch.shader.locs[SHADER_LOC_MATRIX_MODEL] != -1)
        rlSetUniformMatrix(batch.shader.locs[SHADER_LOC_MATRIX_MODEL], MatrixIdentity());

      if (batch.shader.locs[SHADER_LOC_COLOR_DIFFUSE] != -1)
        rlSetUniform(batch.shader.locs[SHADER_LOC_COLOR_DIFFUSE], white, RL_SHADER_UNIFORM_VEC4, 1);

      if (batch.tex_loc != -1 && batch.texture.id > 0)
      {
        rlActiveTextureSlot(0);
        rlEnableTexture(batch.texture.id);
        rlSetUniform(batch.tex_loc, &slot, RL_SHADER_UNIFORM_INT, 1);
      }

      // Draw only the vertex ranges belonging to visible PVS clusters
      rlEnableVertexArray(batch.mesh.vaoId);
      for (auto &[vis_key, range] : batch.cluster_ranges)
      {
        if (visible_vis_keys.count(vis_key))
        {
          rlDrawVertexArray(range.offset, range.count);
          last_draw_count++;
        }
      }
      rlDisableVertexArray();

      if (batch.texture.id > 0)
        rlDisableTexture();
      rlDisableShader();
    }
  }

  /*
  DrawAll
  draws all geometry in the map (no culling)
  */
  void DrawAll(Shader &shader, bool enable_wireframe)
  {
    total_model_count = 0;
    for (Model model : all_models)
    {
      total_model_count++;
      model.materials[0].shader = shader;
      DrawModel(model, {}, 1.f, WHITE);

      // Wireframe mode
      if (!enable_wireframe)
        return;
      model.materials[0].shader = {rlGetShaderIdDefault(), rlGetShaderLocsDefault()};
      DrawModelWires(model, {}, 1.f, BLACK);
    }
  };

  /*
  CleanUp
  call when done with bsp map to unload all its data
  */
  void CleanUp()
  {
    // clean models
    for (auto &model : all_models)
      UnloadModel(model);
    all_models.clear();

    // clean textures
    for (auto &[name, tex] : textures)
      UnloadTexture(tex);
    textures.clear();

    // clean pvs data
    faces_by_texture.clear();
    face_to_texture.clear();
    all_nodes.clear();
    all_leaves.clear();
    leaf_to_cluster.clear();
    pvs.compressed_data.clear();

    // unload texture batches (one mesh per texture)
    for (auto &batch : texture_batches)
      UnloadMesh(batch.mesh);
    texture_batches.clear();

    bsp_file.reset();
    file_stream.reset();
  };
};

inline BSP_Renderer bsp_renderer; // global BSP renderer

// -----------------------------------------------------------------------
// BSP Collisions
// -----------------------------------------------------------------------

struct BSP_Collider
{
  std::vector<Plane> planes;
  std::vector<Clipnode> clipnodes;
  int32_t root = 0;

  /*
  Load
  loads bsp collider data from BSP_File struct
  */
  void Load(BSP_File &map)
  {
    root = map.model(0).clipnode1_id;
    int np = map.header.planes.size / sizeof(Plane);
    int nc = map.header.clipnodes.size / sizeof(Clipnode);
    for (int i = 0; i < np; i++)
      planes.push_back(map.plane(i));
    for (int i = 0; i < nc; i++)
      clipnodes.push_back(map._read<Clipnode>(map.header.clipnodes, i));
  };

  /*
  PointContents
  look through bsp tree to find a given point
  */
  int PointContents(Vector3 p)
  {
    int node = root;

    // start at root node of bsp tree
    while (node >= 0)
    {
      // at each node, check which side the point lands on after splitting the map
      const Clipnode &cn = clipnodes[node];
      const Plane &pl = planes[cn.planenum];
      float d = Vector3DotProduct(pl.normal, p) - pl.dist;

      // go down tree until we hit a leaf (node < 0)
      node = (d >= 0) ? cn.front : cn.back;
    }
    // -2 = SOLID (inside a wall)
    // -1 = EMPTY (outside of map/ in tha void)
    // Other negative values = different types of BRUSHES (water,lava,etc)
    return node;
  }

  /*
  IsSolid
  returns true/false if point is inside a solid
  Only returns true if the SOLID is marked -2 , as shown above -2 represents SOLID geometry
  */
  bool IsSolid(Vector3 raylib_pos)
  {
    // return PointContents(ToQuake(raylib_pos)) == -2;
    auto tr = TraceBSP(raylib_pos, raylib_pos);
    return tr.started_solid || tr.all_solid;
  };

  /*
  RecursiveHullCheck
  */
  bool RecursiveHullCheck(int node, float p1f, float p2f, Vector3 p1, Vector3 p2, TraceResult &tr)
  {
    if (node < 0)
    {
      if (node == -2)
      {
        if (p1f == 0.0f)
          tr.started_solid = true;
        return false;
      }
      return true;
    }

    const Clipnode &cn = clipnodes[node];
    const Plane &pl = planes[cn.planenum];

    float t1 = Vector3DotProduct(pl.normal, p1) - pl.dist;
    float t2 = Vector3DotProduct(pl.normal, p2) - pl.dist;

    if (t1 >= 0 && t2 >= 0)
      return RecursiveHullCheck(cn.front, p1f, p2f, p1, p2, tr);
    if (t1 < 0 && t2 < 0)
      return RecursiveHullCheck(cn.back, p1f, p2f, p1, p2, tr);

    static constexpr float DIST_EPSILON = 0.03f;

    int side = (t1 < 0) ? 1 : 0;
    int near_child = side ? cn.back : cn.front;
    int far_child = side ? cn.front : cn.back;

    float frac, frac2;
    if (t1 < 0)
    {
      frac = std::clamp((t1 + DIST_EPSILON) / (t1 - t2), 0.0f, 1.0f);
      frac2 = std::clamp((t1 - DIST_EPSILON) / (t1 - t2), 0.0f, 1.0f);
    }
    else
    {
      frac = std::clamp((t1 - DIST_EPSILON) / (t1 - t2), 0.0f, 1.0f);
      frac2 = std::clamp((t1 + DIST_EPSILON) / (t1 - t2), 0.0f, 1.0f);
    }

    float midf = p1f + (p2f - p1f) * frac;
    float midf2 = p1f + (p2f - p1f) * frac2;
    Vector3 mid = Vector3Lerp(p1, p2, frac);
    Vector3 mid2 = Vector3Lerp(p1, p2, frac2);

    // near side
    if (!RecursiveHullCheck(near_child, p1f, midf, p1, mid, tr))
      return false;

    // far side — starts at mid2, not mid, to avoid overlapping the near trace
    if (!RecursiveHullCheck(far_child, midf2, p2f, mid2, p2, tr))
    {
      if (!tr.started_solid)
      {
        tr.fraction = midf;
        Vector3 qnorm = (side == 0) ? pl.normal : Vector3Negate(pl.normal);
        tr.normal = Vector3Normalize({qnorm.y, qnorm.z, qnorm.x});
      }
      return false;
    }

    return true;
  };

  /*
  TraceBox
  traces the BSP player hull from 'from' to 'to' (raylib space).
  the clipnodes are pre-expanded for the player size, so this is a proper swept-box test.
  */
  inline TraceResult TraceBSP(Vector3 from_rl, Vector3 to_rl)
  {
    TraceResult tr = {};
    tr.fraction = 1.0f;
    RecursiveHullCheck(root, 0.0f, 1.0f, ToQuake(from_rl), ToQuake(to_rl), tr);

    if (tr.started_solid && tr.fraction == 1.0f)
      tr.all_solid = true;
    return tr;
  };

  // -----------------------------------------------------------------------
  // Entity Hull Collisions
  // -----------------------------------------------------------------------

  // Clipnode root indices for non-trigger brush entity submodels.
  // These hulls are already pre-expanded for the player size (hull 1),
  // same as the world hull — so no Minkowski expansion needed.
  std::vector<int32_t> entity_hull_roots;

  /*
  TraceEntityHulls
  Runs RecursiveHullCheck against every registered entity hull and returns
  the earliest hit, combining with the world-geometry trace in TraceCombined.
  */
  TraceResult TraceEntityHulls(Vector3 from, Vector3 to)
  {
    TraceResult best;
    best.fraction = 1.0f;

    Vector3 qfrom = ToQuake(from);
    Vector3 qto = ToQuake(to);

    for (int32_t ent_root : entity_hull_roots)
    {
      TraceResult tr = {};
      tr.fraction = 1.0f;
      RecursiveHullCheck(ent_root, 0.0f, 1.0f, qfrom, qto, tr);
      if (tr.started_solid && tr.fraction == 1.0f)
        tr.all_solid = true;

      if (tr.fraction < best.fraction)
        best = tr;
    }
    return best;
  };

  /*
  TraceCombined
  Returns the earliest hit between BSP world geometry and solid brush entities.
  Use this everywhere instead of TraceBSP so entities block movement.
  */
  TraceResult TraceCombined(Vector3 from, Vector3 to)
  {
    TraceResult bsp = TraceBSP(from, to);
    TraceResult ent = TraceEntityHulls(from, to);
    return (ent.fraction < bsp.fraction) ? ent : bsp;
  };

  /*
  ClipVelocity
  clips scaled velocity
  */
#define STOP_EPS 0.1
  int ClipVelocity(Vector3 in, Vector3 normal, Vector3 &out, float overbounce)
  {
    int blocked = 0;

    if (normal.y > SLOPE_MAX)
      blocked |= 1;
    if (fabsf(normal.y) < SLOPE_MAX)
      blocked |= 2;

    float backoff = Vector3DotProduct(in, normal) * overbounce;

    out.x = in.x - (normal.x * backoff);
    out.y = in.y - (normal.y * backoff);
    out.z = in.z - (normal.z * backoff);

    float eps = STOP_EPS * bsp_raylib_scale;
    if (out.x > -eps && out.x < eps)
      out.x = 0;
    if (out.y > -eps && out.y < eps)
      out.y = 0;
    if (out.z > -eps && out.z < eps)
      out.z = 0;

    return blocked;
  };

  /*
  NudgePosition
  nudges the player in a scale dir to their pos
  */
  void NudgePosition(Vector3 &player_pos)
  {
    if (!IsSolid(player_pos))
      return;

    // small nudge distance — just enough to escape the surface
    const float nudge = 0.125f * bsp_raylib_scale;

    // try cardinal directions first, prioritize up
    static const Vector3 dirs[] = {
        {0, 1, 0}, // up first
        {0, -1, 0},
        {1, 0, 0},
        {-1, 0, 0},
        {0, 0, 1},
        {0, 0, -1},
    };

    for (auto &d : dirs)
    {
      Vector3 test = {
          player_pos.x + d.x * nudge,
          player_pos.y + d.y * nudge,
          player_pos.z + d.z * nudge};
      if (!IsSolid(test))
      {
        player_pos = test;
        return;
      }
    }
  }

  /*
  CategorizePosition
  figures out what type of thing we are colliding with or are on top of
  */
  void CategorizePosition(Vector3 &pos, Vector3 &vel, bool &is_grounded)
  {
    if (vel.y > 50.0f * bsp_raylib_scale)
    {
      is_grounded = false;
      return;
    }

    float down_dist = 2.0f * bsp_raylib_scale; // step height downwards
    Vector3 point_down = {pos.x, pos.y - down_dist, pos.z};
    TraceResult tr = TraceCombined(pos, point_down);

    if (tr.fraction == 1.0f || tr.normal.y < SLOPE_MAX)
    {
      is_grounded = false; // too steep
    }
    else
    {
      is_grounded = true; // found floor
    }
  };

  /*
  ApplyFriction
  applies friction the player movement
  */
  void ApplyFriction(Vector3 &pos, Vector3 &vel, bool is_grounded)
  {
    float speed = Vector3Length(vel);

    if (speed < 1.0f * bsp_raylib_scale)
    {
      vel.x = 0;
      vel.z = 0;
      return;
    }

    float friction = 4.0f;
    float stop_speed = 100.0f * bsp_raylib_scale;

    if (is_grounded)
    {
      float vel_dir_x = vel.x / speed;
      float vel_dir_z = vel.z / speed;
      float offset = 16.0f * bsp_raylib_scale;

      Vector3 start = {pos.x + vel_dir_x * offset, pos.y, pos.z + vel_dir_z * offset};
      Vector3 stop = {start.x, start.y - (34.0f * bsp_raylib_scale), start.z};

      TraceResult tr = TraceCombined(start, stop);

      if (tr.fraction == 1.0f)
      {
        friction *= 2.0f;
      }
    }

    float drop = 0;
    if (is_grounded)
    {
      float control = (speed < stop_speed) ? stop_speed : speed;
      drop = control * friction * GetFrameTime();
    }

    float new_speed = speed - drop;
    if (new_speed < 0)
      new_speed = 0;

    new_speed /= speed;

    vel.x *= new_speed;
    // vel.y *= new_speed;
    vel.z *= new_speed;
  };

  /*
  Accelerate
  Handles acceleration whilst on the ground
  */
  void Accelerate(Vector3 wishdir, float wishspeed, float accel, Vector3 &vel)
  {
    float currentspeed = Vector3DotProduct(vel, wishdir);
    float addspeed = wishspeed - currentspeed;

    if (addspeed <= 0)
      return;

    float accelspeed = accel * GetFrameTime() * wishspeed;

    if (accelspeed > addspeed)
      accelspeed = addspeed;

    vel.x += accelspeed * wishdir.x;
    vel.y += accelspeed * wishdir.y;
    vel.z += accelspeed * wishdir.z;
  };

  /*
  AirAccelerate
  Handles acceleration whilst in the air
  */
  void AirAccelerate(Vector3 wishdir, float wishspeed, float accel, Vector3 &vel)
  {
    float wishspd = (wishspeed > 30.0f * bsp_raylib_scale) ? (30.0f * bsp_raylib_scale) : wishspeed;

    float currentspeed = Vector3DotProduct(vel, wishdir);
    float addspeed = wishspd - currentspeed;

    if (addspeed <= 0)
      return;

    float accelspeed = accel * wishspeed * GetFrameTime();
    if (accelspeed > addspeed)
      accelspeed = addspeed;

    vel.x += accelspeed * wishdir.x;
    vel.y += accelspeed * wishdir.y;
    vel.z += accelspeed * wishdir.z;
  };

  /*
  FlyMove
  Handles Movement Physics for a GameObject in 3D Space
  */
  int FlyMove(Vector3 &pos, Vector3 &vel)
  {
    Vector3 planes[5];
    int numplanes = 0;
    Vector3 original_velocity = vel;
    Vector3 primal_velocity = vel;
    int blocked = 0;
    float time_left = GetFrameTime();

    for (int bumpcount = 0; bumpcount < 4; bumpcount++)
    {
      if (Vector3LengthSqr(vel) == 0)
        break;

      Vector3 end = Vector3Add(pos, Vector3Scale(vel, time_left));
      TraceResult tr = TraceCombined(pos, end);

      if (tr.fraction < 1.0f && !tr.started_solid)
      {
        // printf("hit: normal=(%.2f,%.2f,%.2f) fraction=%.3f\n",
        //  tr.normal.x, tr.normal.y, tr.normal.z, tr.fraction);
      }

      if (tr.all_solid)
      {
        NudgePosition(pos);
        return 3;
      }

      if (tr.started_solid)
      {
        tr.fraction = 0;
        // vel = {0, 0, 0};
      }

      if (tr.fraction > 0)
      {
        float move_frac = tr.fraction - 0.001f;
        if (move_frac < 0)
          move_frac = 0;
        pos = Vector3Add(pos, Vector3Scale(vel, time_left * move_frac));
        original_velocity = vel;
        numplanes = 0;
      }

      if (tr.fraction == 1.0f)
        break; // finished the move

      if (tr.normal.y > SLOPE_MAX)
        blocked |= 1; // floor
      if (fabsf(tr.normal.y) < SLOPE_MAX)
        blocked |= 2; // wall/Step

      time_left *= (1.0f - tr.fraction);

      if (numplanes >= 5)
      {
        vel = {0, 0, 0};
        break;
      }

      planes[numplanes] = tr.normal;
      numplanes++;

      int i, j;
      for (i = 0; i < numplanes; i++)
      {
        // clip velocity against plane
        float bounce = (planes[i].y > SLOPE_MAX) ? 1.0f : 1.01f;
        ClipVelocity(original_velocity, planes[i], vel, bounce); // 1.01 helps push out of walls and not get snagged

        // check if hits OTHER planes
        for (j = 0; j < numplanes; j++)
          if (j != i)
          {
            if (Vector3DotProduct(vel, planes[j]) < 0)
              break; // hits another plane
          }
        if (j == numplanes)
          break; // found a direction that clears all planes
      }

      if (i != numplanes)
      {
      }
      else
      {
        if (numplanes != 2)
        {
          vel = {0, 0, 0};
          break;
        }
        Vector3 dir = Vector3CrossProduct(planes[0], planes[1]);
        float d = Vector3DotProduct(dir, vel);
        vel = Vector3Scale(dir, d);
      }

      if (Vector3DotProduct(vel, primal_velocity) <= 0)
      {
        vel = {0, 0, 0};
        break;
      }
    }
    return blocked;
  };

  /*
  GroundMove
  Handles ground physics for player (accel,frict, etc)
  */
  void GroundMove(Vector3 &pos, Vector3 &vel)
  {
    Vector3 original_pos = pos;
    Vector3 original_vel = vel;
    if (vel.y < 0)
      vel.y = 0;

    float step_height = 18.f * bsp_raylib_scale;

    // try just moving to final dest
    Vector3 direct_dest = {
        pos.x + vel.x * GetFrameTime(),
        pos.y,
        pos.z + vel.z * GetFrameTime()};
    TraceResult direct_tr = TraceCombined(pos, direct_dest);

    if (direct_tr.fraction == 1.0f)
    {
      pos = direct_dest; // no walls, keep moving
      Vector3 down_dest = {pos.x, pos.y - step_height, pos.z};
      TraceResult down_tr = TraceCombined(pos, down_dest);
      if (down_tr.fraction < 1.0f && down_tr.normal.y > SLOPE_MAX)
      {
        pos.y -= step_height * down_tr.fraction;
        pos.y += 0.03125f * bsp_raylib_scale;
      }
      return;
    }

    // if the horizontal trace hit a floor/slope (not a wall), just slide — don't step
    if (direct_tr.normal.y > SLOPE_MAX)
    {
      vel.y = Vector3Length({vel.x, 0, vel.z}) * SLOPE_BOOST; // SLOPE BOOST
      FlyMove(pos, vel);
      vel.y = 0; // don't carry upward slope velocity into next frame's ground check
      return;
    }

    // hit wall , try step up
    // first get slide result
    int slide_blocked = FlyMove(pos, vel);
    Vector3 slide_pos = pos;
    Vector3 slide_vel = vel;

    // only step up if slide actually hit a wall, not just a floor/slope
    if (!(slide_blocked & 2))
      return;

    pos = original_pos;
    vel = original_vel;
    vel.y = 0;

    // step up
    Vector3 dest_up = {pos.x, pos.y + step_height, pos.z};
    TraceResult tr_up = TraceCombined(pos, dest_up);
    if (!tr_up.started_solid)
    {
      pos.y += step_height * tr_up.fraction;
    }

    // step forward (slide in air)
    FlyMove(pos, vel);

    // step down (to floor)
    Vector3 step_pos = {};
    Vector3 air_finish_pos = pos;
    Vector3 dest_down = {air_finish_pos.x, air_finish_pos.y - step_height, air_finish_pos.z};
    TraceResult tr_down = TraceCombined(air_finish_pos, dest_down);
    float slide_dist_sq, step_dist_sq;
    bool stepped_up;

    // if we land on wall/steep slop ignore this
    if (tr_down.normal.y < SLOPE_MAX)
      goto usedown;

    if (!tr_down.started_solid)
    {
      pos.y = air_finish_pos.y - step_height * tr_down.fraction;
      pos.y += 0.03125f * bsp_raylib_scale;
    }

    step_pos = pos;
    // which went further?
    {
      slide_dist_sq = powf(slide_pos.x - original_pos.x, 2) + powf(slide_pos.z - original_pos.z, 2);
      step_dist_sq = powf(step_pos.x - original_pos.x, 2) + powf(step_pos.z - original_pos.z, 2);

      stepped_up = (step_pos.y > original_pos.y + 0.01f * bsp_raylib_scale);
      if (step_dist_sq < 0.0001f || !stepped_up)
        goto usedown;

      if (slide_dist_sq >= step_dist_sq)
      {
      usedown:
        pos = slide_pos;
        vel = slide_vel;
      }
      else
      {
        // step move
        // use step pos, but keep vertical velocity from side move
        pos = step_pos;
        vel.y = slide_vel.y;
      }
    }
  };

  /*
  AirMove
  Calculates movement for a player
  */
  void AirMove(Vector3 &pos, Vector3 &vel, Vector3 &forward, Vector3 &right, float fmove, float smove)
  {
    float fmove_s = fmove * bsp_raylib_scale;
    float smove_s = smove * bsp_raylib_scale;
    Vector3 wishvel = {
        forward.x * fmove_s + right.x * smove_s,
        0,
        forward.z * fmove_s + right.z * smove_s};

    float wishspeed = Vector3Length(wishvel);
    Vector3 wishdir = (wishspeed > 0) ? Vector3Normalize(wishvel) : (Vector3){0, 0, 0};

    float max_speed = 320.0f * bsp_raylib_scale;
    if (wishspeed > max_speed)
      wishspeed = max_speed;

    const float gravity = 800.0f * bsp_raylib_scale;

    if (is_grounded && vel.y <= 0)
    {
      Accelerate(wishdir, wishspeed, 10.0f, vel);
      vel.y -= gravity * GetFrameTime();
      GroundMove(pos, vel);
    }
    else
    {
      AirAccelerate(wishdir, wishspeed, 1.0f, vel);
      vel.y -= gravity * GetFrameTime();
      FlyMove(pos, vel);
    }
  };

  /*

  */
  void PlayerJump(Vector3 &vel, bool is_grounded)
  {
    if (is_grounded && (IsKeyPressed(KEY_SPACE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)))
    {
      vel.y = 270.0f * bsp_raylib_scale;
    }
  };

  void StepDown(Vector3 &pos, Vector3 vel, bool is_grounded)
  {
    if (is_grounded && vel.y <= 0)
    {
      float step = 18.0f * bsp_raylib_scale;
      Vector3 down = {pos.x, pos.y - step, pos.z};
      TraceResult tr = TraceCombined(pos, down);
      if (tr.fraction < 1.0f && tr.normal.y > SLOPE_MAX)
      {
        pos.y -= step * tr.fraction;
        pos.y += 0.03125f * bsp_raylib_scale;
      }
    }
  };

  /*
  MoveAndSlide
  Call on a players position (after moving) to detect bsp collisions
  */
  static inline bool is_grounded = false;
  Vector3 MoveAndSlide(Vector3 pos, Vector3 &vel, Vector3 forward, Vector3 right, float fmove, float smove)
  {
    is_grounded = false;

    NudgePosition(pos);
    CategorizePosition(pos, vel, is_grounded);
    ApplyFriction(pos, vel, is_grounded);
    AirMove(pos, vel, forward, right, fmove, smove);
    PlayerJump(vel, is_grounded);
    StepDown(pos, vel, is_grounded);
    CategorizePosition(pos, vel, is_grounded);

    return pos;
  };

  /*
  IsGrounded
  returns true or false if the thing is on the ground
  */
  bool IsGrounded() { return is_grounded; };
};

inline BSP_Collider bsp_collider; // stores all bsp collision data for a map

/*
LoadModelsFromBSPFile
loads all renderable models from a BSP file, grouped by texture to minimize draw calls.
loads PVS data for visibility culling.
*/
inline std::vector<Model> LoadModelsFromBSPFile(const std::filesystem::path &path)
{
  // load bsp file
  bsp_renderer.LoadFromFile(path);

  // load bsp collision data
  std::ifstream bsp_file{path, std::ios::binary};
  BSP_File map{bsp_file};
  bsp_collider.entity_hull_roots.clear(); // clear stale entity hulls from any previous map
  bsp_collider.Load(map);

  // copy models to models vector
  models = bsp_renderer.all_models;

  return models;
}

// -----------------------------------------------------------------------
// Brush Entity Spawning
// -----------------------------------------------------------------------

/*
BSP_BrushEntityData
All data extracted from a single brush entity (func_door, trigger_once, etc.)
Pass this to your game's object factory to create GameObjects.
*/
struct BSP_BrushEntityData
{
  std::string classname;
  std::unordered_map<std::string, std::string> tags; // all raw entity key-values
  Vector3 origin = {0, 0, 0};                        // parsed + converted to raylib space
  Model model = {};                                  // brush geometry (check meshCount > 0)
  bool has_model = false;
  Vector3 collision_box = {0, 0, 0};
  Vector3 collision_offset = {0, 0, 0};

  // Convenience: read a tag or return a default
  std::string GetTag(const std::string &key, const std::string &fallback = "") const
  {
    auto it = tags.find(key);
    return it != tags.end() ? it->second : fallback;
  }
};

/*
BSP_SpawnBrushEntities
Iterates the entity list, finds every brush entity (any entity whose "model" key
starts with "*"), builds its geometry, and returns a BSP_BrushEntityData for each.

Call this after LoadModelsFromBSPFile. Iterate the result and push_back your
own GameObject subclass into `gameobjects` for each entry.

Example:
  for (auto &data : BSP_SpawnBrushEntities())
  {
    if (data.classname == "func_door")
    {
      auto door = std::make_unique<MyDoor>(data);
      gameobjects.push_back(std::move(door));
    }
  }
*/
inline std::vector<BSP_BrushEntityData> BSP_SpawnBrushEntities()
{
  std::vector<BSP_BrushEntityData> results;
  if (!bsp_renderer.bsp_file)
    return results;

  BSP_File &bsp = *bsp_renderer.bsp_file;

  for (auto &entity : bsp.entities())
  {
    // must have a classname
    if (!entity.tags.count("classname"))
      continue;

    // must reference a brush submodel ("*1", "*2", etc.)
    if (!entity.tags.count("model"))
      continue;
    const std::string &model_key = entity.tags.at("model");
    if (model_key.empty() || model_key[0] != '*')
      continue;

    int model_idx = 0;
    try
    {
      model_idx = std::stoi(model_key.substr(1));
    }
    catch (...)
    {
      continue;
    }

    BSP_BrushEntityData data;
    data.classname = entity.tags.at("classname");
    data.tags = entity.tags;

    // parse origin ("x y z" in Quake space) → raylib space
    if (entity.tags.count("origin"))
    {
      float qx = 0, qy = 0, qz = 0;
      sscanf(entity.tags.at("origin").c_str(), "%f %f %f", &qx, &qy, &qz);
      data.origin = FromQuake({qx, qy, qz});
    }

    // collect faces for this submodel
    BSP_Model submodel = bsp.model(model_idx);
    std::vector<Face> faces;
    faces.reserve(submodel.face_num);
    for (int f = 0; f < submodel.face_num; f++)
      faces.push_back(bsp.face(submodel.face_id + f));

    if (!faces.empty())
    {
      Mesh mesh = GenMeshFaces(bsp, faces);
      data.model = LoadModelFromMesh(mesh);

      // assign shader — triggers are invisible, everything else uses default
      bool is_trigger = data.classname.starts_with("trigger");
      // data.model.materials[0].shader = is_trigger ? invisible_shader : default_shader;
      // data.model.materials[0].shader = debug_shader;

      // assign texture from the first face (best-effort; submodels can have mixed textures)
      TexInfo ti = bsp.texinfo(faces[0].texinfo_id);
      Miptex mx = bsp.miptex(ti.miptex_id);
      auto tex_it = bsp_renderer.textures.find(std::string(mx.name));
      if (tex_it != bsp_renderer.textures.end())
        data.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex_it->second;

      // compute bounding box data (vertices are in world-space raylib coords)
      BoundingBox bb = GetModelBoundingBox(data.model);
      data.collision_box = Vector3Subtract(bb.max, bb.min);
      Vector3 bb_center = Vector3Scale(Vector3Add(bb.min, bb.max), 0.5f);
      data.collision_offset = Vector3Subtract(bb_center, data.origin);

      // register solid (non-trigger) entities with the collider so MoveAndSlide blocks on them.
      // use clipnode1_id — the BSP hull already pre-expanded for the player size, same as the world hull.
      if (!is_trigger)
        bsp_collider.entity_hull_roots.push_back(submodel.clipnode1_id);

      data.has_model = true;
    }

    results.push_back(std::move(data));
  }

  return results;
}

/*
BSP_Draw
draws all loaded BSP models with the given shader with optional wireframe overlay.
*/
inline void BSP_Draw(Shader &shader, bool enable_wireframe, Vector3 camera_pos = {0, 0, 0})
{
  // bsp_renderer.DrawAll(shader, enable_wireframe);
  bsp_renderer.DrawWithPVS(shader, camera_pos, enable_wireframe);
};

/*
BSP_DrawDebug
*/
inline void BSP_DrawDebug(Vector3 camera_pos)
{
  DrawText(TextFormat("PVS cluster draws: %d / %d  (%d tex batches)",
                      bsp_renderer.last_draw_count,
                      bsp_renderer.total_model_count,
                      (int)bsp_renderer.texture_batches.size()),
           0, 20, 20, GREEN);
  DrawText(TextFormat("Leaf: %d  Cluster: %d",
                      bsp_renderer.FindLeaf(camera_pos),
                      bsp_renderer.leaf_to_cluster[bsp_renderer.FindLeaf(camera_pos)]),
           0, 40, 20, GREEN);
};

/*
BSP_CleanUp
unloads all BSP models
*/
inline void BSP_CleanUp()
{
  bsp_renderer.CleanUp();
  std::for_each(models.begin(), models.end(), UnloadModel);
  models.clear();
};

// -----------------------------------------------------------------------
// Palette
// -----------------------------------------------------------------------

/*
palette
contains the palette.lmp data to be used for textures.
BSP files index into this 256-color table to resolve final RGB pixel values.
*/
inline Color_RGB8 palette(uint8_t id)
{
  static const Color_RGB8 _PALETTE[] = {
      {0, 0, 0},
      {15, 15, 15},
      {31, 31, 31},
      {47, 47, 47},
      {63, 63, 63},
      {75, 75, 75},
      {91, 91, 91},
      {107, 107, 107},
      {123, 123, 123},
      {139, 139, 139},
      {155, 155, 155},
      {171, 171, 171},
      {187, 187, 187},
      {203, 203, 203},
      {219, 219, 219},
      {235, 235, 235},
      {15, 11, 7},
      {23, 15, 11},
      {31, 23, 11},
      {39, 27, 15},
      {47, 35, 19},
      {55, 43, 23},
      {63, 47, 23},
      {75, 55, 27},
      {83, 59, 27},
      {91, 67, 31},
      {99, 75, 31},
      {107, 83, 31},
      {115, 87, 31},
      {123, 95, 35},
      {131, 103, 35},
      {143, 111, 35},
      {11, 11, 15},
      {19, 19, 27},
      {27, 27, 39},
      {39, 39, 51},
      {47, 47, 63},
      {55, 55, 75},
      {63, 63, 87},
      {71, 71, 103},
      {79, 79, 115},
      {91, 91, 127},
      {99, 99, 139},
      {107, 107, 151},
      {115, 115, 163},
      {123, 123, 175},
      {131, 131, 187},
      {139, 139, 203},
      {0, 0, 0},
      {7, 7, 0},
      {11, 11, 0},
      {19, 19, 0},
      {27, 27, 0},
      {35, 35, 0},
      {43, 43, 7},
      {47, 47, 7},
      {55, 55, 7},
      {63, 63, 7},
      {71, 71, 7},
      {75, 75, 11},
      {83, 83, 11},
      {91, 91, 11},
      {99, 99, 11},
      {107, 107, 15},
      {7, 0, 0},
      {15, 0, 0},
      {23, 0, 0},
      {31, 0, 0},
      {39, 0, 0},
      {47, 0, 0},
      {55, 0, 0},
      {63, 0, 0},
      {71, 0, 0},
      {79, 0, 0},
      {87, 0, 0},
      {95, 0, 0},
      {103, 0, 0},
      {111, 0, 0},
      {119, 0, 0},
      {127, 0, 0},
      {19, 19, 0},
      {27, 27, 0},
      {35, 35, 0},
      {47, 43, 0},
      {55, 47, 0},
      {67, 55, 0},
      {75, 59, 7},
      {87, 67, 7},
      {95, 71, 7},
      {107, 75, 11},
      {119, 83, 15},
      {131, 87, 19},
      {139, 91, 19},
      {151, 95, 27},
      {163, 99, 31},
      {175, 103, 35},
      {35, 19, 7},
      {47, 23, 11},
      {59, 31, 15},
      {75, 35, 19},
      {87, 43, 23},
      {99, 47, 31},
      {115, 55, 35},
      {127, 59, 43},
      {143, 67, 51},
      {159, 79, 51},
      {175, 99, 47},
      {191, 119, 47},
      {207, 143, 43},
      {223, 171, 39},
      {239, 203, 31},
      {255, 243, 27},
      {11, 7, 0},
      {27, 19, 0},
      {43, 35, 15},
      {55, 43, 19},
      {71, 51, 27},
      {83, 55, 35},
      {99, 63, 43},
      {111, 71, 51},
      {127, 83, 63},
      {139, 95, 71},
      {155, 107, 83},
      {167, 123, 95},
      {183, 135, 107},
      {195, 147, 123},
      {211, 163, 139},
      {227, 179, 151},
      {171, 139, 163},
      {159, 127, 151},
      {147, 115, 135},
      {139, 103, 123},
      {127, 91, 111},
      {119, 83, 99},
      {107, 75, 87},
      {95, 63, 75},
      {87, 55, 67},
      {75, 47, 55},
      {67, 39, 47},
      {55, 31, 35},
      {43, 23, 27},
      {35, 19, 19},
      {23, 11, 11},
      {15, 7, 7},
      {187, 115, 159},
      {175, 107, 143},
      {163, 95, 131},
      {151, 87, 119},
      {139, 79, 107},
      {127, 75, 95},
      {115, 67, 83},
      {107, 59, 75},
      {95, 51, 63},
      {83, 43, 55},
      {71, 35, 43},
      {59, 31, 35},
      {47, 23, 27},
      {35, 19, 19},
      {23, 11, 11},
      {15, 7, 7},
      {219, 195, 187},
      {203, 179, 167},
      {191, 163, 155},
      {175, 151, 139},
      {163, 135, 123},
      {151, 123, 111},
      {135, 111, 95},
      {123, 99, 83},
      {107, 87, 71},
      {95, 75, 59},
      {83, 63, 51},
      {67, 51, 39},
      {55, 43, 31},
      {39, 31, 23},
      {27, 19, 15},
      {15, 11, 7},
      {111, 131, 123},
      {103, 123, 111},
      {95, 115, 103},
      {87, 107, 95},
      {79, 99, 87},
      {71, 91, 79},
      {63, 83, 71},
      {55, 75, 63},
      {47, 67, 55},
      {43, 59, 47},
      {35, 51, 39},
      {31, 43, 31},
      {23, 35, 23},
      {15, 27, 19},
      {11, 19, 11},
      {7, 11, 7},
      {255, 243, 27},
      {239, 223, 23},
      {219, 203, 19},
      {203, 183, 15},
      {187, 167, 15},
      {171, 151, 11},
      {155, 131, 7},
      {139, 115, 7},
      {123, 99, 7},
      {107, 83, 0},
      {91, 71, 0},
      {75, 55, 0},
      {59, 43, 0},
      {43, 31, 0},
      {27, 15, 0},
      {11, 7, 0},
      {0, 0, 255},
      {11, 11, 239},
      {19, 19, 223},
      {27, 27, 207},
      {35, 35, 191},
      {43, 43, 175},
      {47, 47, 159},
      {47, 47, 143},
      {47, 47, 127},
      {47, 47, 111},
      {47, 47, 95},
      {43, 43, 79},
      {35, 35, 63},
      {27, 27, 47},
      {19, 19, 31},
      {11, 11, 15},
      {43, 0, 0},
      {59, 0, 0},
      {75, 7, 0},
      {95, 7, 0},
      {111, 15, 0},
      {127, 23, 7},
      {147, 31, 7},
      {163, 39, 11},
      {183, 51, 15},
      {195, 75, 27},
      {207, 99, 43},
      {219, 127, 59},
      {227, 151, 79},
      {231, 171, 95},
      {239, 191, 119},
      {247, 211, 139},
      {167, 123, 59},
      {183, 155, 55},
      {199, 195, 55},
      {231, 227, 87},
      {127, 191, 255},
      {171, 231, 255},
      {215, 255, 255},
      {103, 0, 0},
      {139, 0, 0},
      {179, 0, 0},
      {215, 0, 0},
      {255, 0, 0},
      {255, 243, 147},
      {255, 247, 199},
      {255, 255, 255},
      {159, 91, 83},
  };
  return _PALETTE[id];
};
