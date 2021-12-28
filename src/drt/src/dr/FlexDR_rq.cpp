/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dr/FlexDR.h"
#include "frRTree.h"
#include "serialization.h"

using namespace std;
using namespace fr;

// I believe it is safe not to sort the query results here despite the
// use of the serializer.  Most of the use of the query is in init()
// which we call pre-serialization.  The only post-serialization use I
// see is in the graphics which don't care about order (it's just
// drawing the shapes).

struct FlexDRWorkerRegionQuery::Impl
{
  FlexDRWorker* drWorker;
  std::vector<RTree<drConnFig*>> shapes_;  // only for drXXX in dr worker

  static void add(
      drConnFig* connFig,
      std::vector<std::vector<rq_box_value_t<drConnFig*>>>& allShapes);

 private:
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    (ar) & drWorker;
    (ar) & shapes_;
  }

  friend class boost::serialization::access;
};

FlexDRWorkerRegionQuery::FlexDRWorkerRegionQuery(FlexDRWorker* in)
    : impl_(make_unique<Impl>())
{
  impl_->drWorker = in;
}

FlexDRWorkerRegionQuery::~FlexDRWorkerRegionQuery() = default;

void FlexDRWorkerRegionQuery::cleanup()
{
  impl_->shapes_.clear();
  impl_->shapes_.shrink_to_fit();
}

void FlexDRWorkerRegionQuery::add(drConnFig* connFig)
{
  Rect frb;
  if (connFig->typeId() == drcPathSeg || connFig->typeId() == frcRect
      || connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drShape*>(connFig);
    obj->getBBox(frb);
    impl_->shapes_.at(obj->getLayerNum()).insert(make_pair(frb, obj));
  } else if (connFig->typeId() == drcVia) {
    auto via = static_cast<drVia*>(connFig);
    dbTransform xform;
    Point origin;
    via->getOrigin(origin);
    xform.setOffset(origin);
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        impl_->shapes_.at(via->getViaDef()->getLayer1Num())
            .insert(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        impl_->shapes_.at(via->getViaDef()->getLayer2Num())
            .insert(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        impl_->shapes_.at(via->getViaDef()->getCutLayerNum())
            .insert(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
  } else {
    cout << "Error: unsupported region query add" << endl;
  }
}

void FlexDRWorkerRegionQuery::Impl::add(
    drConnFig* connFig,
    vector<vector<rq_box_value_t<drConnFig*>>>& allShapes)
{
  Rect frb;
  if (connFig->typeId() == drcPathSeg || connFig->typeId() == frcRect
      || connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drShape*>(connFig);
    obj->getBBox(frb);
    allShapes.at(obj->getLayerNum()).push_back(make_pair(frb, obj));
  } else if (connFig->typeId() == drcVia) {
    auto via = static_cast<drVia*>(connFig);
    dbTransform xform;
    Point origin;
    via->getOrigin(origin);
    xform.setOffset(origin);
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        allShapes.at(via->getViaDef()->getLayer1Num())
            .push_back(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        allShapes.at(via->getViaDef()->getLayer2Num())
            .push_back(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        allShapes.at(via->getViaDef()->getCutLayerNum())
            .push_back(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query add" << endl;
      }
    }
  } else {
    cout << "Error: unsupported region query add" << endl;
  }
}

void FlexDRWorkerRegionQuery::remove(drConnFig* connFig)
{
  Rect frb;
  if (connFig->typeId() == drcPathSeg || connFig->typeId() == frcRect
      || connFig->typeId() == drcPatchWire) {
    auto obj = static_cast<drShape*>(connFig);
    obj->getBBox(frb);
    impl_->shapes_.at(obj->getLayerNum()).remove(make_pair(frb, obj));
  } else if (connFig->typeId() == drcVia) {
    auto via = static_cast<drVia*>(connFig);
    dbTransform xform;
    Point origin;
    via->getOrigin(origin);
    xform.setOffset(origin);
    for (auto& uShape : via->getViaDef()->getLayer1Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        impl_->shapes_.at(via->getViaDef()->getLayer1Num())
            .remove(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query remove" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getLayer2Figs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        impl_->shapes_.at(via->getViaDef()->getLayer2Num())
            .remove(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query remove" << endl;
      }
    }
    for (auto& uShape : via->getViaDef()->getCutFigs()) {
      auto shape = uShape.get();
      if (shape->typeId() == frcRect) {
        shape->getBBox(frb);
        xform.apply(frb);
        impl_->shapes_.at(via->getViaDef()->getCutLayerNum())
            .remove(make_pair(frb, via));
      } else {
        cout << "Error: unsupported region query remove" << endl;
      }
    }
  } else {
    cout << "Error: unsupported region query remove" << endl;
  }
}

void FlexDRWorkerRegionQuery::query(const Rect& box,
                                    const frLayerNum layerNum,
                                    vector<drConnFig*>& result) const
{
  vector<rq_box_value_t<drConnFig*>> temp;
  impl_->shapes_.at(layerNum).query(bgi::intersects(box), back_inserter(temp));
  result.reserve(temp.size());
  transform(temp.begin(), temp.end(), back_inserter(result), [](auto& kv) {
    return kv.second;
  });
}

void FlexDRWorkerRegionQuery::query(
    const Rect& box,
    const frLayerNum layerNum,
    vector<rq_box_value_t<drConnFig*>>& result) const
{
  impl_->shapes_.at(layerNum).query(bgi::intersects(box),
                                    back_inserter(result));
}

void FlexDRWorkerRegionQuery::init()
{
  int numLayers = impl_->drWorker->getTech()->getLayers().size();
  impl_->shapes_.clear();
  impl_->shapes_.resize(numLayers);
  vector<vector<rq_box_value_t<drConnFig*>>> allShapes(numLayers);
  for (auto& net : impl_->drWorker->getNets()) {
    for (auto& connFig : net->getRouteConnFigs()) {
      impl_->add(connFig.get(), allShapes);
    }
    for (auto& connFig : net->getExtConnFigs()) {
      impl_->add(connFig.get(), allShapes);
    }
  }
  for (auto i = 0; i < numLayers; i++) {
    impl_->shapes_.at(i) = boost::move(RTree<drConnFig*>(allShapes.at(i)));
    allShapes.at(i).clear();
    allShapes.at(i).shrink_to_fit();
  }
}

bool FlexDRWorkerRegionQuery::isEmpty() const
{
  return impl_->shapes_.empty();
}

template <class Archive>
void FlexDRWorkerRegionQuery::serialize(Archive& ar, const unsigned int version)
{
  (ar) & impl_;
}

// Explicit instantiations
template void FlexDRWorkerRegionQuery::serialize<InputArchive>(
    InputArchive& ar,
    const unsigned int file_version);

template void FlexDRWorkerRegionQuery::serialize<OutputArchive>(
    OutputArchive& ar,
    const unsigned int file_version);
