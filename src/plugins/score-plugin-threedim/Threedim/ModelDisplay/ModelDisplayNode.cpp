#include "ModelDisplayNode.hpp"

#include <Gfx/Graph/GeometryFilterNodeRenderer.hpp>
#include <Gfx/Graph/NodeRenderer.hpp>
#include <Gfx/Graph/RenderList.hpp>
#include <Gfx/Graph/RenderState.hpp>
#include <boost/algorithm/string.hpp>
#include <ossia/detail/fmt.hpp>
#include <ossia/detail/math.hpp>
#include <ossia/gfx/port_index.hpp>
#include <ossia/network/value/value.hpp>
#include <ossia/network/value/value_conversion.hpp>
#include <score/tools/Debug.hpp>
#include <score/tools/SafeCast.hpp>

#include <QPainter>

#if defined(near)
#undef near
#undef far
#endif

namespace score::gfx
{

#define model_display_default_uniforms \
  "\
layout(std140, binding = 0) uniform renderer_t { \n\
  mat4 clipSpaceCorrMatrix; \n\
  vec2 renderSize; \n\
} renderer; \n\
 \n\
// Time-dependent uniforms, only relevant during execution \n\
layout(std140, binding = 1) uniform process_t { \n\
  float TIME; \n\
  float TIMEDELTA; \n\
  float PROGRESS; \n\
 \n\
  int PASSINDEX; \n\
  int FRAMEINDEX; \n\
 \n\
  vec4 DATE; \n\
  vec4 MOUSE; \n\
  vec4 CHANNELTIME; \n\
 \n\
  float SAMPLERATE; \n\
} isf_process_uniforms; \n\
 \n\
layout(std140, binding = 2) uniform material_t { \n\
  mat4 matrixModelViewProjection; \n\
  mat4 matrixModelView; \n\
  mat4 matrixModel; \n\
  mat4 matrixView; \n\
  mat4 matrixProjection; \n\
  mat3 matrixNormal; \n\
} mat; \n\
 \n\
float TIME = isf_process_uniforms.TIME; \n\
float TIMEDELTA = isf_process_uniforms.TIMEDELTA; \n\
float PROGRESS = isf_process_uniforms.PROGRESS; \n\
int PASSINDEX = isf_process_uniforms.PASSINDEX; \n\
int FRAMEINDEX = isf_process_uniforms.FRAMEINDEX; \n\
vec4 DATE = isf_process_uniforms.DATE; \n\
"

const constexpr auto vtx_output_triangle = R"_(
out gl_PerVertex {
vec4 gl_Position;
};
)_";
const constexpr auto vtx_output_point = R"_(
out gl_PerVertex {
vec4 gl_Position;
float gl_PointSize;
};
)_";
const constexpr auto vtx_output_process_triangle = R"_()_";
const constexpr auto vtx_output_process_point = R"_(
  gl_PointSize = 1.0f;
)_";

const constexpr auto model_display_vertex_shader_phong = R"_(#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 esVertex;
layout(location = 1) out vec3 esNormal;
layout(location = 2) out vec2 v_texcoord;

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

%vtx_define_filters%

%vtx_output%

void main()
{
  vec3 in_position = position;
  vec3 in_normal = normal;
  vec2 in_uv = texcoord;
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(1);

  %vtx_do_filters%

  esVertex = in_position;
  esNormal = in_normal;
  v_texcoord = in_uv;
  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);

  %vtx_output_process%
}
)_";

const constexpr auto model_display_fragment_shader_phong = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(binding=3) uniform sampler2D y_tex;

layout(location = 0) in vec3 esVertex;
layout(location = 1) in vec3 esNormal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 0) out vec4 fragColor;

vec4 lightPosition = vec4(100, 10, 10, 0.); // should be in the eye space
vec4 lightAmbient = vec4(0.1, 0.1, 0.1, 1); // light ambient color
vec4 lightDiffuse = vec4(0.0, 0.2, 0.7, 1); // light diffuse color
vec4 lightSpecular = vec4(0.9, 0.9, 0.9, 1); // light specular color
vec4 materialAmbient= vec4(0.1, 0.4, 0, 1); // material ambient color
vec4 materialDiffuse= vec4(0.2, 0.8, 0, 1); // material diffuse color
vec4 materialSpecular= vec4(0, 0, 1, 1); // material specular color
float materialShininess = 0.5; // material specular shininess

void main ()
{
    vec3 normal = normalize(esNormal);
    vec3 light;
    lightPosition.y = sin(TIME) * 20.;
    lightPosition.z = cos(TIME) * 50.;
    if(lightPosition.w == 0.0)
    {
        light = normalize(lightPosition.xyz);
    }
    else
    {
        light = normalize(lightPosition.xyz - esVertex);
    }
    vec3 view = normalize(-esVertex);
    vec3 halfv = normalize(light + view);

    vec3 color = lightAmbient.rgb * materialAmbient.rgb;        // begin with ambient
    float dotNL = max(dot(normal, light), 0.0);
    color += lightDiffuse.rgb * materialDiffuse.rgb * dotNL;    // add diffuse
    // color *= texture2D(map0, texCoord0).rgb;                    // modulate texture map
    float dotNH = max(dot(normal, halfv), 0.0);
    color += pow(dotNH, materialShininess) * lightSpecular.rgb * materialSpecular.rgb; // add specular


    vec4 tex = texture(y_tex, v_texcoord);
    fragColor = vec4(mix(color, tex.rgb, 0.5), materialDiffuse.a);
}
)_";

const constexpr auto model_display_vertex_shader_texcoord = R"_(#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;

layout(location = 0) out vec2 v_texcoord;

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

%vtx_define_filters%

%vtx_output%

void main()
{
  vec3 in_position = position;
  vec3 in_normal = vec3(0);
  vec2 in_uv = texcoord;
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(1);

  %vtx_do_filters%

  v_texcoord = in_uv;
  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);
  %vtx_output_process%
}
)_";

const constexpr auto model_display_fragment_shader_texcoord = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(binding=3) uniform sampler2D y_tex;

layout(location = 0) in vec2 v_texcoord;
layout(location = 0) out vec4 fragColor;

void main ()
{
  fragColor = texture(y_tex, v_texcoord);
}
)_";

// See also:
// https://www.pbr-book.org/3ed-2018/Texture/Texture_Coordinate_Generation#fragment-TextureMethodDefinitions-3
// https://gamedevelopment.tutsplus.com/articles/use-tri-planar-texture-mapping-for-better-terrain--gamedev-13821

const constexpr auto model_display_vertex_shader_triplanar = R"_(#version 450
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec3 v_coords;

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

%vtx_define_filters%

%vtx_output%

void main()
{
  vec3 in_position = position;
  vec3 in_normal = normal;
  vec2 in_uv = vec2(0);
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(1);

  %vtx_do_filters%

  v_normal = in_normal;
  v_coords = (mat.matrixModel * vec4(in_position.xyz, 1.0)).xyz;
  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);
  %vtx_output_process%
}
)_";

const constexpr auto model_display_fragment_shader_triplanar = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec3 v_coords;
layout(location = 0) out vec4 fragColor;

void main ()
{
  vec3 blending = abs( v_normal );
  blending = normalize(max(blending, 0.00001)); // Force weights to sum to 1.0
  float b = (blending.x + blending.y + blending.z);
  blending /= vec3(b, b, b);

  float scale = 0.1;

  vec4 xaxis = texture(y_tex, v_coords.yz * scale);
  vec4 yaxis = texture(y_tex, v_coords.xz * scale);
  vec4 zaxis = texture(y_tex, v_coords.xy * scale);
  vec4 tex = xaxis * blending.x + xaxis * blending.y + zaxis * blending.z;

  fragColor = tex;
}
)_";

const constexpr auto model_display_vertex_shader_spherical = R"_(#version 450
layout(location = 0) in vec3 position;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 v_e;
layout(location = 1) out vec3 v_n;

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

%vtx_define_filters%

%vtx_output%

void main()
{
  vec3 in_position = position;
  vec3 in_normal = normal;
  vec2 in_uv = vec2(0);
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(1);

  %vtx_do_filters%

  // https://www.clicktorelease.com/blog/creating-spherical-environment-mapping-shader.html
  vec4 p = vec4( in_position, 1. );
  v_e = normalize( vec3( mat.matrixModelView * p ) );
  v_n = normal; //normalize( mat.matrixNormal * in_normal );
  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);
  %vtx_output_process%
}
)_";
const constexpr auto model_display_fragment_shader_spherical = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

layout(location = 0) in vec3 v_e;
layout(location = 1) in vec3 v_n;
layout(location = 0) out vec4 fragColor;

const float pi = 3.14159265359;

float atan2(in float y, in float x)
{
    bool s = (abs(x) > abs(y));
    return mix(3.141596/2.0 - atan(x,y), atan(y,x), s);
}
void main ()
{
  vec2 uv = vec2(atan2(v_n.z, v_n.x), asin(v_n.y));
  uv = uv * vec2(1. / 2. * pi, 1. / pi) + 0.5;
  fragColor = texture(y_tex, uv);
}
)_";

const constexpr auto model_display_vertex_shader_spherical2 = R"_(#version 450
layout(location = 0) in vec3 position;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 v_e;
layout(location = 1) out vec3 v_n;

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

%vtx_define_filters%

%vtx_output%

float atan2(in float y, in float x)
{
    bool s = (abs(x) > abs(y));
    return mix(3.141596/2.0 - atan(x,y), atan(y,x), s);
}

void main()
{
  vec3 in_position = position;
  vec3 in_normal = normal;
  vec2 in_uv = vec2(0);
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(1);

  %vtx_do_filters%

  // https://www.clicktorelease.com/blog/creating-spherical-environment-mapping-shader.html
  vec4 p = vec4( in_position, 1. );
  v_e = normalize( vec3( mat.matrixModelView * p ) );
  v_n = normalize( mat.matrixNormal * in_normal );
  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);
  %vtx_output_process%
}
)_";
const constexpr auto model_display_fragment_shader_spherical2 = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

layout(location = 0) in vec3 v_e;
layout(location = 1) in vec3 v_n;
layout(location = 0) out vec4 fragColor;

void main ()
{
  vec3 r = reflect( v_e, v_n );
  float m = 2. * sqrt( pow( r.x, 2. ) + pow( r.y, 2. ) + pow( r.z + 1., 2. ) );
  vec2 vN = r.xy / m + .5;

  fragColor = texture(y_tex, vN.xy);
}
)_";

const constexpr auto model_display_vertex_shader_viewspace = R"_(#version 450
layout(location = 0) in vec3 position;

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

%vtx_define_filters%

%vtx_output%

void main()
{
  vec3 in_position = position;
  vec3 in_normal = vec3(0);
  vec2 in_uv = vec2(0);
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(1);

  %vtx_do_filters%

  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);
  %vtx_output_process%
}
)_";

const constexpr auto model_display_fragment_shader_viewspace = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

layout(location = 0) out vec4 fragColor;

void main ()
{
  fragColor = texture(y_tex, gl_FragCoord.xy / renderer.renderSize.xy);
}
)_";

const constexpr auto model_display_vertex_shader_barycentric = R"_(#version 450
layout(location = 0) in vec3 position;

layout(location = 1) out vec2 v_bary;

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

%vtx_define_filters%

%vtx_output%

void main()
{
  vec3 in_position = position;
  vec3 in_normal = vec3(0);
  vec2 in_uv = vec2(0);
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(1);

  %vtx_do_filters%

  if(gl_VertexIndex % 3 == 0) v_bary = vec2(0, 0);
  else if(gl_VertexIndex % 3 == 1) v_bary = vec2(0, 1);
  else if(gl_VertexIndex % 3 == 2) v_bary = vec2(1, 0);

  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);
  %vtx_output_process%
}
)_";

const constexpr auto model_display_fragment_shader_barycentric = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(binding = 3) uniform sampler2D y_tex;

layout(location = 1) in vec2 v_bary;
layout(location = 0) out vec4 fragColor;

void main ()
{
  fragColor = texture(y_tex, v_bary);
}
)_";

const constexpr auto model_display_vertex_shader_color = R"_(#version 450
layout(location = 0) in vec3 position;
layout(location = 2) in vec3 color;

layout(location = 0) out vec3 v_color;

)_" model_display_default_uniforms R"_(

%vtx_define_filters%

%vtx_output%

void main()
{
  vec3 in_position = position;
  vec3 in_normal = vec3(0);
  vec2 in_uv = vec2(0);
  vec3 in_tangent = vec3(0);
  vec4 in_color = vec4(color.rgb, 1.);

  %vtx_do_filters%

  v_color = in_color.rgb;
  gl_Position = renderer.clipSpaceCorrMatrix * mat.matrixModelViewProjection * vec4(in_position.xyz, 1.0);

  %vtx_output_process%
}
)_";

const constexpr auto model_display_fragment_shader_color = R"_(#version 450

)_" model_display_default_uniforms R"_(

layout(location = 0) in vec3 v_color;
layout(location = 0) out vec4 fragColor;

void main ()
{
  fragColor.rgb = v_color;
  fragColor.a = 1.;
}
)_";

ModelDisplayNode::ModelDisplayNode()
{
  input.push_back(new Port{this, nullptr, Types::Image, {}});
  input.push_back(new Port{this, nullptr, Types::Geometry, {}});
  input.push_back(new Port{this, nullptr, Types::Camera, {}});
  output.push_back(new Port{this, {}, Types::Image, {}});

  m_materialData.reset((char*)&ubo);
}

ModelDisplayNode::~ModelDisplayNode()
{
  m_materialData.release();
}

#include <Gfx/Qt5CompatPush> // clang-format: keep
class ModelDisplayNode::Renderer : public GenericNodeRenderer
{
public:
  using GenericNodeRenderer::GenericNodeRenderer;

  struct RenderShaders
  {
    QShader phongVS, phongFS;
    QShader texCoordVS, texCoordFS;
    QShader triplanarVS, triplanarFS;
    QShader sphericalVS, sphericalFS;
    QShader spherical2VS, spherical2FS;
    QShader viewspaceVS, viewspaceFS;
    QShader barycentricVS, barycentricFS;
    QShader colorVS, colorFS;
  } triangle, point;

  int m_curShader{0};
  int m_draw_mode{0};
  int64_t materialChangedIndex{-1};
  int64_t geometryChangedIndex{-1};
  int64_t meshChangedIndex{-1};

private:
  ~Renderer() = default;

  score::gfx::TextureRenderTarget m_inputTarget;
  TextureRenderTarget renderTargetForInput(const Port& p) override
  {
    return m_inputTarget;
  }

  void initPasses_impl(RenderList& renderer, const Mesh& mesh, RenderShaders& shaders)
  {
    auto& n = (ModelDisplayNode&)node;
    bool has_texcoord = mesh.flags() & Mesh::HasTexCoord;
    bool has_normals = mesh.flags() & Mesh::HasNormals;
    bool has_colors = mesh.flags() & Mesh::HasColor;

    int cur_binding = 4;
    std::vector<QRhiBuffer*> ubos;
    std::vector<QRhiShaderResourceBinding> additional_bindings;

    if (mesh.filters)
    {
      if (!mesh.filters->filters.empty())
      {
        for (auto& f : mesh.filters->filters)
        {
          for (auto n : renderer.renderers)
          {
            if (n->id == f.node_id)
            {
              if (auto c = safe_cast<score::gfx::GeometryFilterNodeRenderer*>(n))
              {
                additional_bindings.push_back(QRhiShaderResourceBinding::uniformBuffer(
                    cur_binding, QRhiShaderResourceBinding::VertexStage, c->material()));

                cur_binding++;
                break;
              }
            }
          }
        }
      }
    }

    if (has_colors && n.wantedProjection == 7)
    {
      defaultPassesInit(
          renderer, mesh, shaders.colorVS, shaders.colorFS, additional_bindings);
      return;
    }

    if (has_texcoord && has_normals)
    {
      switch (n.wantedProjection)
      {
        default:
        case 0: // Needs TCoord
          defaultPassesInit(
              renderer,
              mesh,
              shaders.texCoordVS,
              shaders.texCoordFS,
              additional_bindings);
          break;
        case 1: // Needs Normals
          defaultPassesInit(
              renderer,
              mesh,
              shaders.triplanarVS,
              shaders.triplanarFS,
              additional_bindings);
          break;
        case 2: // Needs Normals
          defaultPassesInit(
              renderer,
              mesh,
              shaders.sphericalVS,
              shaders.sphericalFS,
              additional_bindings);
          break;
        case 3: // Needs Normals
          defaultPassesInit(
              renderer,
              mesh,
              shaders.spherical2VS,
              shaders.spherical2FS,
              additional_bindings);
          break;
        case 4: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.viewspaceVS,
              shaders.viewspaceFS,
              additional_bindings);
          break;
        case 5: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.barycentricVS,
              shaders.barycentricFS,
              additional_bindings);
          break;
        case 6: // Needs TCoord + Normals
          defaultPassesInit(
              renderer, mesh, shaders.phongVS, shaders.phongFS, additional_bindings);
          break;
      }
    }
    else if (has_texcoord && !has_normals)
    {
      switch (n.wantedProjection)
      {
        default:
        case 0:
        case 1:
        case 2:
        case 3:
        case 6:
          defaultPassesInit(
              renderer,
              mesh,
              shaders.texCoordVS,
              shaders.texCoordFS,
              additional_bindings);
          break;
        case 4: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.viewspaceVS,
              shaders.viewspaceFS,
              additional_bindings);
          break;
        case 5: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.barycentricVS,
              shaders.barycentricFS,
              additional_bindings);
          break;
      }
    }
    else if (has_normals && !has_texcoord)
    {
      switch (n.wantedProjection)
      {
        default:
        case 0:
        case 6:
        case 1: // Needs Normals
          defaultPassesInit(
              renderer,
              mesh,
              shaders.triplanarVS,
              shaders.triplanarFS,
              additional_bindings);
          break;
        case 2: // Needs Normals
          defaultPassesInit(
              renderer,
              mesh,
              shaders.sphericalVS,
              shaders.sphericalFS,
              additional_bindings);
          break;
        case 3: // Needs Normals
          defaultPassesInit(
              renderer,
              mesh,
              shaders.spherical2VS,
              shaders.spherical2FS,
              additional_bindings);
          break;
        case 4: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.viewspaceVS,
              shaders.viewspaceFS,
              additional_bindings);
          break;
        case 5: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.barycentricVS,
              shaders.barycentricFS,
              additional_bindings);
          break;
      }
    }
    else if (!has_texcoord && !has_normals)
    {
      switch (n.wantedProjection)
      {
        default:
        case 0:
        case 1:
        case 2:
        case 3:
        case 6:
        case 4: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.viewspaceVS,
              shaders.viewspaceFS,
              additional_bindings);
          break;
        case 5: // Needs just position
          defaultPassesInit(
              renderer,
              mesh,
              shaders.barycentricVS,
              shaders.barycentricFS,
              additional_bindings);
          break;
      }
    }
  }

  void initPasses(RenderList& renderer, const Mesh& mesh)
  {
    auto& n = (ModelDisplayNode&)node;

    createShaders(renderer, mesh);
    m_curShader = n.wantedProjection;
    m_draw_mode = n.draw_mode;
    switch (m_draw_mode)
    {
      case 0:
        initPasses_impl(renderer, mesh, triangle);
        for (auto& [e, pass] : this->m_p)
        {
          pass.pipeline->destroy();
          pass.pipeline->setTopology(QRhiGraphicsPipeline::Triangles);
          pass.pipeline->create();
        }
        break;
      case 1:
        initPasses_impl(renderer, mesh, point);
        for (auto& [e, pass] : this->m_p)
        {
          pass.pipeline->destroy();
          pass.pipeline->setTopology(QRhiGraphicsPipeline::Points);
          pass.pipeline->create();
        }
        break;
      case 2:
        initPasses_impl(renderer, mesh, triangle);
        for (auto& [e, pass] : this->m_p)
        {
          pass.pipeline->destroy();
          pass.pipeline->setTopology(QRhiGraphicsPipeline::Lines);
          pass.pipeline->create();
        }
        break;
    }
  }

  QString processVertexShader(
      QString init,
      std::string_view out,
      std::string_view proc,
      const score::gfx::Mesh& mesh)
  {

    std::string vtx_define_filters;
    std::string vtx_do_filters;
    // Add additional bindings.
    // 0: renderer
    // 1: processUBO
    // 2: materialUBO
    // 3: input texture
    // 4: it starts here :)
    int cur_binding = 4;

    if (mesh.filters)
    {
      if (!mesh.filters->filters.empty())
      {
        for (auto& f : mesh.filters->filters)
        {
          auto shader = f.shader;
          boost::algorithm::replace_first(
              shader, "%next%", std::to_string(cur_binding++));
          vtx_define_filters += shader;
          vtx_do_filters += fmt::format(
              "process_vertex_{}(in_position, in_normal, in_uv, in_tangent, "
              "in_color);",
              f.filter_id);
        }
      }
    }

    init.replace("%vtx_define_filters%", vtx_define_filters.data());
    init.replace("%vtx_do_filters%", vtx_do_filters.data());
    init.replace("%vtx_output%", out.data());
    init.replace("%vtx_output_process%", proc.data());
    return init;
  }

  void createShaders(RenderList& renderer, const score::gfx::Mesh& mesh)
  {
    const QString triangle_phongVS = processVertexShader(
        model_display_vertex_shader_phong,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);
    const QString triangle_texcoordVS = processVertexShader(
        model_display_vertex_shader_texcoord,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);
    const QString triangle_triplanarVS = processVertexShader(
        model_display_vertex_shader_triplanar,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);
    const QString triangle_sphericalVS = processVertexShader(
        model_display_vertex_shader_spherical,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);
    const QString triangle_spherical2VS = processVertexShader(
        model_display_vertex_shader_spherical2,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);
    const QString triangle_viewspaceVS = processVertexShader(
        model_display_vertex_shader_viewspace,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);
    const QString triangle_barycentricVS = processVertexShader(
        model_display_vertex_shader_barycentric,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);
    const QString triangle_colorVS = processVertexShader(
        model_display_vertex_shader_color,
        vtx_output_triangle,
        vtx_output_process_triangle,
        mesh);

    std::tie(triangle.phongVS, triangle.phongFS) = score::gfx::makeShaders(
        renderer.state, triangle_phongVS, model_display_fragment_shader_phong);
    std::tie(triangle.texCoordVS, triangle.texCoordFS) = score::gfx::makeShaders(
        renderer.state, triangle_texcoordVS, model_display_fragment_shader_texcoord);
    std::tie(triangle.triplanarVS, triangle.triplanarFS) = score::gfx::makeShaders(
        renderer.state, triangle_triplanarVS, model_display_fragment_shader_triplanar);
    std::tie(triangle.sphericalVS, triangle.sphericalFS) = score::gfx::makeShaders(
        renderer.state, triangle_sphericalVS, model_display_fragment_shader_spherical);
    std::tie(triangle.spherical2VS, triangle.spherical2FS) = score::gfx::makeShaders(
        renderer.state, triangle_spherical2VS, model_display_fragment_shader_spherical2);
    std::tie(triangle.viewspaceVS, triangle.viewspaceFS) = score::gfx::makeShaders(
        renderer.state, triangle_viewspaceVS, model_display_fragment_shader_viewspace);
    std::tie(triangle.barycentricVS, triangle.barycentricFS) = score::gfx::makeShaders(
        renderer.state,
        triangle_barycentricVS,
        model_display_fragment_shader_barycentric);
    std::tie(triangle.colorVS, triangle.colorFS) = score::gfx::makeShaders(
        renderer.state, triangle_colorVS, model_display_fragment_shader_color);

    const QString point_phongVS = processVertexShader(
        model_display_vertex_shader_phong,
        vtx_output_point,
        vtx_output_process_point,
        mesh);
    const QString point_texcoordVS = processVertexShader(
        model_display_vertex_shader_texcoord,
        vtx_output_point,
        vtx_output_process_point,
        mesh);
    const QString point_triplanarVS = processVertexShader(
        model_display_vertex_shader_triplanar,
        vtx_output_point,
        vtx_output_process_point,
        mesh);
    const QString point_sphericalVS = processVertexShader(
        model_display_vertex_shader_spherical,
        vtx_output_point,
        vtx_output_process_point,
        mesh);
    const QString point_spherical2VS = processVertexShader(
        model_display_vertex_shader_spherical2,
        vtx_output_point,
        vtx_output_process_point,
        mesh);
    const QString point_viewspaceVS = processVertexShader(
        model_display_vertex_shader_viewspace,
        vtx_output_point,
        vtx_output_process_point,
        mesh);
    const QString point_barycentricVS = processVertexShader(
        model_display_vertex_shader_barycentric,
        vtx_output_point,
        vtx_output_process_point,
        mesh);
    const QString point_colorVS = processVertexShader(
        model_display_vertex_shader_color,
        vtx_output_point,
        vtx_output_process_point,
        mesh);

    std::tie(point.phongVS, point.phongFS) = score::gfx::makeShaders(
        renderer.state, point_phongVS, model_display_fragment_shader_phong);
    std::tie(point.texCoordVS, point.texCoordFS) = score::gfx::makeShaders(
        renderer.state, point_texcoordVS, model_display_fragment_shader_texcoord);
    std::tie(point.triplanarVS, point.triplanarFS) = score::gfx::makeShaders(
        renderer.state, point_triplanarVS, model_display_fragment_shader_triplanar);
    std::tie(point.sphericalVS, point.sphericalFS) = score::gfx::makeShaders(
        renderer.state, point_sphericalVS, model_display_fragment_shader_spherical);
    std::tie(point.spherical2VS, point.spherical2FS) = score::gfx::makeShaders(
        renderer.state, point_spherical2VS, model_display_fragment_shader_spherical2);
    std::tie(point.viewspaceVS, point.viewspaceFS) = score::gfx::makeShaders(
        renderer.state, point_viewspaceVS, model_display_fragment_shader_viewspace);
    std::tie(point.barycentricVS, point.barycentricFS) = score::gfx::makeShaders(
        renderer.state, point_barycentricVS, model_display_fragment_shader_barycentric);
    std::tie(point.colorVS, point.colorFS) = score::gfx::makeShaders(
        renderer.state, point_colorVS, model_display_fragment_shader_color);
  }
  void init(RenderList& renderer, QRhiResourceUpdateBatch& res) override
  {
    auto& rhi = *renderer.state.rhi;
    // Sampler for input texture

    auto sampler = rhi.newSampler(
        QRhiSampler::Linear,
        QRhiSampler::Linear,
        QRhiSampler::Linear,
        QRhiSampler::Mirror,
        QRhiSampler::Mirror);
    sampler->setName("ModelDisplayNode::init::sampler");
    SCORE_ASSERT(sampler->create());

    m_inputTarget = score::gfx::createRenderTarget(
        renderer.state,
        QRhiTexture::RGBA8,
        renderer.state.renderSize,
        renderer.samples(),
        QRhiTexture::MipMapped | QRhiTexture::UsedWithGenerateMips);

    auto texture = m_inputTarget.texture;
    m_samplers.push_back({sampler, texture});

    const auto& mesh = m_mesh ? *m_mesh : renderer.defaultQuad();
    defaultMeshInit(renderer, mesh, res);
    processUBOInit(renderer);
    m_material.init(renderer, node.input, m_samplers);

    initPasses(renderer, mesh);
  }

  template <std::size_t N>
  static void fromGL(float (&from)[N], auto& to)
  {
    memcpy(to.data(), from, sizeof(float[N]));
  }
  template <std::size_t N>
  static void toGL(auto& from, float (&to)[N])
  {
    memcpy(to, from.data(), sizeof(float[N]));
  }

  void update(RenderList& renderer, QRhiResourceUpdateBatch& res) override
  {
    auto& n = (ModelDisplayNode&)node;

    bool mustRecreatePasses = false;
    if (n.hasMaterialChanged(materialChangedIndex))
    {
      QMatrix4x4 model{};
      fromGL(n.ubo.model, model);

      QMatrix4x4 projection;
      // FIXME should use the size of the target instead
      // Since we can render on multiple target, this means that we must have one UBO for each
      projection.perspective(
          n.fov,
          qreal(renderer.state.renderSize.width()) / renderer.state.renderSize.height(),
          n.near,
          n.far);
      QMatrix4x4 view;

      view.lookAt(
          QVector3D{n.position[0], n.position[1], n.position[2]},
          QVector3D{n.center[0], n.center[1], n.center[2]},
          QVector3D{0, 1, 0});
      QMatrix4x4 mv = view * model;
      QMatrix4x4 mvp = projection * view * model;
      QMatrix3x3 norm = model.normalMatrix();

      ModelCameraUBO* mc = &n.ubo;
      std::fill_n((char*)mc, sizeof(ModelCameraUBO), 0);
      toGL(model, mc->model);
      toGL(projection, mc->projection);
      toGL(view, mc->view);
      toGL(mv, mc->mv);
      toGL(mvp, mc->mvp);
      toGL(norm, mc->modelNormal);

      res.updateDynamicBuffer(m_material.buffer, 0, sizeof(ModelCameraUBO), mc);

      if (m_curShader != n.wantedProjection)
        mustRecreatePasses = true;
      if (m_draw_mode != n.draw_mode)
        mustRecreatePasses = true;
    }

    res.updateDynamicBuffer(
        m_processUBO, 0, sizeof(ProcessUBO), &this->node.standardUBO);

    if (node.hasGeometryChanged(geometryChangedIndex))
    {
      if (node.geometry.meshes)
      {
        std::tie(m_mesh, m_meshbufs)
            = renderer.acquireMesh(node.geometry, res, m_mesh, m_meshbufs);
        SCORE_ASSERT(m_mesh);
        this->meshChangedIndex = this->m_mesh->dirtyGeometryIndex;
      }
      mustRecreatePasses = true;
    }

    const auto& mesh = m_mesh ? *m_mesh : renderer.defaultQuad();
    if (mesh.hasGeometryChanged(meshChangedIndex))
      mustRecreatePasses = true;

    if (mustRecreatePasses)
    {
      for (auto& pass : m_p)
        pass.second.release();
      m_p.clear();

      const auto& mesh = m_mesh ? *m_mesh : renderer.defaultQuad();
      initPasses(renderer, mesh);
    }

    res.generateMips(this->m_inputTarget.texture);
  }

  void runRenderPass(RenderList& renderer, QRhiCommandBuffer& cb, Edge& edge) override
  {
    const auto& mesh = m_mesh ? *m_mesh : renderer.defaultQuad();
    defaultRenderPass(renderer, mesh, cb, edge);
  }

  void release(RenderList& r) override
  {
    m_inputTarget.release();
    defaultRelease(r);
  }
};
#include <Gfx/Qt5CompatPop> // clang-format: keep

NodeRenderer* ModelDisplayNode::createRenderer(RenderList& r) const noexcept
{
  return new Renderer{*this};
}

void ModelDisplayNode::process(Message&& msg)
{
  ProcessNode::process(msg.token);

  int32_t p = 0;
  for (const gfx_input& m : msg.input)
  {
    if (auto val = ossia::get_if<ossia::value>(&m))
    {
      switch (p)
      {
        case 2:
        {
          this->position = ossia::convert<ossia::vec3f>(*val);
          this->materialChange();
          break;
        }
        case 3:
        {
          this->center = ossia::convert<ossia::vec3f>(*val);
          this->materialChange();
          break;
        }
        case 4:
        {
          this->fov = ossia::convert<float>(*val);
          this->materialChange();
          break;
        }
        case 5:
        {
          this->near = ossia::convert<float>(*val);
          this->materialChange();
          break;
        }
        case 6:
        {
          this->far = ossia::convert<float>(*val);
          this->materialChange();
          break;
        }
        case 7:
        {
          this->wantedProjection = ossia::convert<int>(*val);
          this->materialChange();
          break;
        }
        case 8:
        {
          this->draw_mode = ossia::convert<int>(*val);
          this->materialChange();
          break;
        }
      }

      p++;
    }
    else if (auto val = ossia::get_if<ossia::geometry_spec>(&m))
    {
      ProcessNode::process(p, *val);

      p++;
    }
    else if (auto val = ossia::get_if<ossia::transform3d>(&m))
    {
      memcpy(this->ubo.model, val->matrix, sizeof(val->matrix));
      this->materialChange();
    }
    else
    {
      p++;
    }
  }
}
}