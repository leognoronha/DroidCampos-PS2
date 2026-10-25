#include <cassert>
#include <iostream>
#include "renderer/3d/pipeline/static/static_pipeline.hpp"
using namespace Tyra;
int main() {
  RendererCore renderer;StaticPipeline pipeline;pipeline.setRenderer(&renderer);
  Vec4 vertices[3],uv[3];MeshMaterialFrame materialFrame;
  materialFrame.vertices=vertices;materialFrame.textureCoords=uv;
  MeshMaterial material;material.textureName="opaque";material.frames={&materialFrame};
  BBox box;MeshFrame frame;frame.bbox=&box;
  StaticMesh mesh;mesh.frame=&frame;mesh.materials={&material};mesh.model.x=-20;
  StaPipOptions options;options.fullClipChecks=true;
  PipelineLightingOptions lights;options.lighting=&lights;
  pipeline.render(&mesh,options);
  assert(pipeline.core.submissions==0 && renderer.texture.repository.lookups==0);
  assert(PipelineDirLightsBag::constructions==0 && !pipeline.isVisible(&mesh));
  options.lighting=nullptr;mesh.model.x=0;
  assert(pipeline.isVisible(&mesh));pipeline.render(&mesh,options);
  assert(pipeline.core.submissions==1 && renderer.texture.repository.lookups==1);
  assert(pipeline.core.lastClip && pipeline.core.lastTexture && !pipeline.core.lastLighting);
  // The same geometry changes visibility when its current model matrix moves.
  mesh.model.x=-20;pipeline.render(&mesh,options);assert(pipeline.core.submissions==1);
  // Camera-space weapon explicitly disables frustum checks.
  options.frustumCulling=PipelineFrustumCulling_None;
  options.fullClipChecks=false;options.transformationType=TyraMP;
  const auto checks=box.checks;pipeline.render(&mesh,options);
  assert(box.checks==checks && pipeline.core.submissions==2 && !pipeline.core.lastClip);
  options.frustumCulling=PipelineFrustumCulling_Simple;options.transformationType=TyraMVP;
  pipeline.render(&mesh,options);assert(pipeline.core.submissions==2);
  mesh.model.x=0;box.classification=IN_FRUSTUM;pipeline.render(&mesh,options);
  assert(pipeline.core.submissions==3);
  std::cout<<"PASS: real StaticPipeline culls before materials/lights, preserves partial/full clipping, moving models and camera-space weapon\n";
}
