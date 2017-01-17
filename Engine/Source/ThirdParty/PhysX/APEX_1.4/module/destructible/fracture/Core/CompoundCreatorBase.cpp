/*
 * Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */


#include "RTdef.h"
#if RT_COMPILE
#include "CompoundCreatorBase.h"
#include <algorithm>
#include <PxAssert.h>

namespace nvidia
{
namespace fracture
{
namespace base
{

using namespace nvidia;

int CompoundCreator::tetFaceIds[4][3] = {{0,1,3},{1,2,3},{2,0,3},{0,2,1}};
int CompoundCreator::tetEdgeVerts[6][2] = {{0,1},{1,2},{2,0},{0,3},{1,3},{2,3}};
int CompoundCreator::tetFaceEdges[4][3] = {{0,4,3},{1,5,4},{2,3,5},{0,2,1}};

#define CONVEX_THRESHOLD (PxPi + 0.05f)

// -----------------------------------------------------------------------------
void CompoundCreator::createTorus(float r0, float r1, int numSegs0, int numSegs1, const PxTransform *trans)
{
	mGeom.clear();
	CompoundGeometry::Convex c;
	PxVec3 nr0,nr1,nz;
	nz = PxVec3(0.0f, 0.0f, 1.0f);
	PxVec3 p,n;
	PxTransform t = PxTransform(PxIdentity);
	if (trans)
		t = *trans;

	nvidia::Array<PxVec3> normals;

	float dphi0 = PxTwoPi / (float)numSegs0;
	float dphi1 = PxTwoPi / (float)numSegs1;
	for (int i = 0; i < numSegs0; i++) {
		nr0 = PxVec3(PxCos(i*dphi0), PxSin(i*dphi0), 0.0f);
		nr1 = PxVec3(PxCos((i+1)*dphi0), PxSin((i+1)*dphi0), 0.0f);
		mGeom.initConvex(c);
		c.numVerts = 2*numSegs1;
		normals.clear();
		for (int j = 0; j < numSegs1; j++) {
			p = nr0 * (r0 + r1 * PxCos(j*dphi1)) + nz * r1 * PxSin(j*dphi1);
			mGeom.vertices.pushBack(t.transform(p));
			n = nr0 * (PxCos(j*dphi1)) + nz * PxSin(j*dphi1);
			normals.pushBack(t.rotate(n));

			p = nr1 * (r0 + r1 * PxCos(j*dphi1)) + nz * r1 * PxSin(j*dphi1);
			mGeom.vertices.pushBack(t.transform(p));
			n = nr1 * (PxCos(j*dphi1)) + nz * PxSin(j*dphi1);
			normals.pushBack(t.rotate(n));
		}

		c.numFaces = 2 + numSegs1;
		mGeom.indices.pushBack(numSegs1); // face size
		mGeom.indices.pushBack(CompoundGeometry::FF_INVISIBLE);        // face flags
		for (int j = 0; j < numSegs1; j++) 
			mGeom.indices.pushBack(2*j);
		mGeom.indices.pushBack(numSegs1); // face size
		mGeom.indices.pushBack(CompoundGeometry::FF_INVISIBLE);		   // face flags
		for (int j = 0; j < numSegs1; j++) {
			mGeom.indices.pushBack(2*(numSegs1-1-j) + 1);
		}
		for (int j = 0; j < numSegs1; j++) {
			int k = (j+1)%numSegs1;
			mGeom.indices.pushBack(4); // face size
			mGeom.indices.pushBack(CompoundGeometry::FF_OBJECT_SURFACE | CompoundGeometry::FF_HAS_NORMALS);
			int i0 = 2*j;
			int i1 = 2*j+1;
			int i2 = 2*k+1;
			int i3 = 2*k;
			mGeom.indices.pushBack(i0);
			mGeom.indices.pushBack(i1);
			mGeom.indices.pushBack(i2);
			mGeom.indices.pushBack(i3);
			mGeom.normals.pushBack(normals[(uint32_t)i0]);
			mGeom.normals.pushBack(normals[(uint32_t)i1]);
			mGeom.normals.pushBack(normals[(uint32_t)i2]);
			mGeom.normals.pushBack(normals[(uint32_t)i3]);
		}
		c.numNeighbors = 2;
		mGeom.neighbors.pushBack((i + (numSegs0-1)) % numSegs0);
		mGeom.neighbors.pushBack((i + 1) % numSegs0);
		mGeom.convexes.pushBack(c);
	}
}

// -----------------------------------------------------------------------------
void CompoundCreator::createCylinder(float r, float h, int numSegs, const PxTransform *trans)
{
	PxTransform t = PxTransform(PxIdentity);
	if (trans)
		t = *trans;

	mGeom.clear();
	CompoundGeometry::Convex c;
	mGeom.initConvex(c);

	float dphi = PxTwoPi / (float)numSegs;
	c.numVerts = 2*numSegs;
	mGeom.vertices.resize((uint32_t)c.numVerts);

	for (int i = 0; i < numSegs; i++) {
		PxVec3 p0(r * PxCos(i*dphi), r * PxSin(i*dphi), -0.5f * h);
		PxVec3 p1(r * PxCos(i*dphi), r * PxSin(i*dphi), 0.5f * h);
		mGeom.vertices[2*(uint32_t)i] = t.transform(p0);
		mGeom.vertices[2*(uint32_t)i+1] = t.transform(p1);
	}

	c.numFaces = 2 + numSegs;

	mGeom.indices.pushBack(numSegs);
	mGeom.indices.pushBack(CompoundGeometry::FF_OBJECT_SURFACE);
	for (int i = 0; i < numSegs; i++) 
		mGeom.indices.pushBack(2*(numSegs-1-i));
	mGeom.indices.pushBack(numSegs);
	mGeom.indices.pushBack(CompoundGeometry::FF_OBJECT_SURFACE);
	for (int i = 0; i < numSegs; i++) 
		mGeom.indices.pushBack(2*i+1);

	for (int i = 0; i < numSegs; i++) {
		int j = (i+1) % numSegs;

		PxVec3 n0(PxCos(i*dphi),PxSin(i*dphi),0.0f);
		PxVec3 n1(PxCos(j*dphi),PxSin(j*dphi),0.0f);

		n0 = t.rotate(n0);
		n1 = t.rotate(n1);
		//n0*=-1;
		//n1*=-1;
		mGeom.indices.pushBack(4);
		mGeom.indices.pushBack(CompoundGeometry::FF_OBJECT_SURFACE | CompoundGeometry::FF_HAS_NORMALS);
		mGeom.indices.pushBack(2*i);
		mGeom.indices.pushBack(2*j);
		mGeom.indices.pushBack(2*j+1);
		mGeom.indices.pushBack(2*i+1);
		mGeom.normals.pushBack(n0);
		mGeom.normals.pushBack(n1);
		mGeom.normals.pushBack(n1);
		mGeom.normals.pushBack(n0);
	}
	mGeom.convexes.pushBack(c);
}

// -----------------------------------------------------------------------------
void CompoundCreator::createBox(const PxVec3 &dims, const PxTransform *trans)
{
	PxTransform t = PxTransform(PxIdentity);
	if (trans)
		t = *trans;

	mGeom.clear();
	CompoundGeometry::Convex c;
	mGeom.initConvex(c);

	c.numVerts = 8;
	mGeom.vertices.clear();
	mGeom.vertices.pushBack(t.transform(PxVec3(-0.5f * dims.x, -0.5f * dims.y, -0.5f * dims.z)));
	mGeom.vertices.pushBack(t.transform(PxVec3( 0.5f * dims.x, -0.5f * dims.y, -0.5f * dims.z)));
	mGeom.vertices.pushBack(t.transform(PxVec3( 0.5f * dims.x,  0.5f * dims.y, -0.5f * dims.z)));
	mGeom.vertices.pushBack(t.transform(PxVec3(-0.5f * dims.x,  0.5f * dims.y, -0.5f * dims.z)));
	mGeom.vertices.pushBack(t.transform(PxVec3(-0.5f * dims.x, -0.5f * dims.y, 0.5f * dims.z)));
	mGeom.vertices.pushBack(t.transform(PxVec3( 0.5f * dims.x, -0.5f * dims.y, 0.5f * dims.z)));
	mGeom.vertices.pushBack(t.transform(PxVec3( 0.5f * dims.x,  0.5f * dims.y, 0.5f * dims.z)));
	mGeom.vertices.pushBack(t.transform(PxVec3(-0.5f * dims.x,  0.5f * dims.y, 0.5f * dims.z)));

	static int faceIds[6][4] = {{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7},{0,3,2,1},{4,5,6,7}};

	c.numFaces = 6;
	for (int i = 0; i < 6; i++) {
		mGeom.indices.pushBack(4);
		mGeom.indices.pushBack(CompoundGeometry::FF_OBJECT_SURFACE);
		for (int j = 0; j < 4; j++) 
			mGeom.indices.pushBack(faceIds[i][j]);
	}
	mGeom.convexes.pushBack(c);
}

// -----------------------------------------------------------------------------
void CompoundCreator::createSphere(const PxVec3 &dims, int resolution, const PxTransform *trans)
{
	PxTransform t = PxTransform(PxIdentity);
	if (trans)
		t = *trans;

	if (resolution < 2) resolution = 2;
	int numSegs0 = 2*resolution;
	int numSegs1 = resolution;

	float rx = 0.5f * dims.x;
	float ry = 0.5f * dims.y;
	float rz = 0.5f * dims.z;

	mGeom.clear();
	CompoundGeometry::Convex c;
	mGeom.initConvex(c);

	float dphi = PxTwoPi / (float)numSegs0;
	float dteta = PxPi / (float)numSegs1;
	PxVec3 p, n;

	for (int i = 1; i < numSegs1; i++) {
		for (int j = 0; j < numSegs0; j++) {
			float phi = j * dphi;
			float teta =  -PxHalfPi + i * dteta;
			p = PxVec3(PxCos(phi)*PxCos(teta), PxSin(phi)*PxCos(teta), PxSin(teta));
			mGeom.vertices.pushBack(PxVec3(p.x * rx, p.y * ry, p.z * rz));
		}
	}
	int bottomNr = (int32_t)mGeom.vertices.size();
	mGeom.vertices.pushBack(PxVec3(0.0f, 0.0f, -rz));
	int topNr = (int32_t)mGeom.vertices.size();
	mGeom.vertices.pushBack(PxVec3(0.0f, 0.0f, +rz));

	c.numVerts = (int32_t)mGeom.vertices.size();

	for (int i = 0; i < numSegs1-2; i++) {
		for (int j = 0; j < numSegs0; j++) {
			mGeom.indices.pushBack(4);		// face size
			mGeom.indices.pushBack(CompoundGeometry::FF_HAS_NORMALS | CompoundGeometry::FF_OBJECT_SURFACE);
			int i0 = i*numSegs0 + j;
			int i1 = i*numSegs0 + (j+1)%numSegs0;
			int i2 = (i+1)*numSegs0 + (j+1)%numSegs0;
			int i3 = (i+1)*numSegs0 + j;
			mGeom.indices.pushBack(i0);
			mGeom.indices.pushBack(i1);
			mGeom.indices.pushBack(i2);
			mGeom.indices.pushBack(i3);
			n = mGeom.vertices[(uint32_t)i0]; n.normalize(); mGeom.normals.pushBack(n);
			n = mGeom.vertices[(uint32_t)i1]; n.normalize(); mGeom.normals.pushBack(n);
			n = mGeom.vertices[(uint32_t)i2]; n.normalize(); mGeom.normals.pushBack(n);
			n = mGeom.vertices[(uint32_t)i3]; n.normalize(); mGeom.normals.pushBack(n);
			c.numFaces++;
		}
	}

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < numSegs0; j++) {
			mGeom.indices.pushBack(3);		// face size
			mGeom.indices.pushBack(CompoundGeometry::FF_HAS_NORMALS | CompoundGeometry::FF_OBJECT_SURFACE);
			int i0,i1,i2;
			if (i == 0) {
				i0 = j;
				i1 = bottomNr;
				i2 = (j+1)%numSegs0;
			}
			else {
				i0 = (numSegs1-2)*numSegs0 + j;
				i1 = (numSegs1-2)*numSegs0 + (j+1)%numSegs0;
				i2 = topNr;
			}
			mGeom.indices.pushBack(i0);
			mGeom.indices.pushBack(i1);
			mGeom.indices.pushBack(i2);
			n = mGeom.vertices[(uint32_t)i0]; n.normalize(); mGeom.normals.pushBack(n);
			n = mGeom.vertices[(uint32_t)i1]; n.normalize(); mGeom.normals.pushBack(n);
			n = mGeom.vertices[(uint32_t)i2]; n.normalize(); mGeom.normals.pushBack(n);
			c.numFaces++;
		}
	}
	mGeom.convexes.pushBack(c);
}

// -----------------------------------------------------------------------------
void CompoundCreator::fromTetraMesh(const nvidia::Array<PxVec3> &tetVerts, const nvidia::Array<int> &tetIndices)
{
	mTetVertices = tetVerts;
	mTetIndices = tetIndices;
	deleteColors();

	computeTetNeighbors();
	computeTetEdges();
	colorTets();
	colorsToConvexes();
}

// -----------------------------------------------------------------------------
void CompoundCreator::computeTetNeighbors()
{
	uint32_t numTets = mTetIndices.size() / 4;

	struct TetFace {
		void init(int i0, int i1, int i2, int faceNr, int tetNr) {
			if (i0 > i1) { int i = i0; i0 = i1; i1 = i; }
			if (i1 > i2) { int i = i1; i1 = i2; i2 = i; }
			if (i0 > i1) { int i = i0; i0 = i1; i1 = i; }
			this->i0 = i0; this->i1 = i1; this->i2 = i2;
			this->faceNr = faceNr; this->tetNr = tetNr;
		}
		bool operator < (const TetFace &f) const {
			if (i0 < f.i0) return true;
			if (i0 > f.i0) return false;
			if (i1 < f.i1) return true;
			if (i1 > f.i1) return false;
			return i2 < f.i2;
		}
		bool operator == (const TetFace &f) const {
			return i0 == f.i0 && i1 == f.i1 && i2 == f.i2;
		}
		int i0,i1,i2;
		int faceNr, tetNr;
	};
	nvidia::Array<TetFace> faces(numTets * 4);

	for (uint32_t i = 0; i < numTets; i++) {
		int ids[4];
		ids[0] = mTetIndices[4*i];
		ids[1] = mTetIndices[4*i+1];
		ids[2] = mTetIndices[4*i+2];
		ids[3] = mTetIndices[4*i+3];
		for (uint32_t j = 0; j < 4; j++) {
			int i0 = ids[(uint32_t)tetFaceIds[j][0]];
			int i1 = ids[(uint32_t)tetFaceIds[j][1]];
			int i2 = ids[(uint32_t)tetFaceIds[j][2]];
			faces[4*i+j].init(i0,i1,i2, (int32_t)j, (int32_t)i); 
		}
	}
	std::sort(faces.begin(), faces.end());

	mTetNeighbors.clear();
	mTetNeighbors.resize(numTets * 4, -1);
	uint32_t i = 0;
	while (i < faces.size()) {
		TetFace &f0 = faces[i];
		i++;
		if (i < faces.size() && faces[i] == f0) {
			TetFace &f1 = faces[i];
			mTetNeighbors[uint32_t(4*f0.tetNr + f0.faceNr)] = f1.tetNr;
			mTetNeighbors[uint32_t(4*f1.tetNr + f1.faceNr)] = f0.tetNr;
			while (i < faces.size() && faces[i] == f0)
				i++;
		}
	}
}

// -----------------------------------------------------------------------------
void CompoundCreator::computeTetEdges()
{
	uint32_t numTets = mTetIndices.size() / 4;

	struct SortEdge {
		void init(int i0, int i1, int edgeNr, int tetNr) {
			if (i0 < i1) { this->i0 = i0; this->i1 = i1; }
			else { this->i0 = i1; this->i1 = i0; }
			this->edgeNr = edgeNr; this->tetNr = tetNr;
		}
		bool operator < (const SortEdge &e) const {
			if (i0 < e.i0) return true;
			if (i0 > e.i0) return false;
			return i1 < e.i1;
		}
		bool operator == (const SortEdge &e) const {
			return i0 == e.i0 && i1 == e.i1;
		}
		int i0,i1;
		int edgeNr, tetNr;
	};
	nvidia::Array<SortEdge> edges(numTets * 6);

	for (uint32_t i = 0; i < numTets; i++) {
		int ids[4];
		ids[0] = mTetIndices[4*i];
		ids[1] = mTetIndices[4*i+1];
		ids[2] = mTetIndices[4*i+2];
		ids[3] = mTetIndices[4*i+3];
		for (uint32_t j = 0; j < 6; j++) {
			int i0 = ids[(uint32_t)tetEdgeVerts[j][0]];
			int i1 = ids[(uint32_t)tetEdgeVerts[j][1]];
			edges[6*i + j].init(i0,i1, (int32_t)j, (int32_t)i);
		}
	}
	std::sort(edges.begin(), edges.end());

	mTetEdgeNrs.clear();
	mTetEdgeNrs.resize(numTets * 6, -1);
	mTetEdges.clear();
	mEdgeTetNrs.clear();
	mEdgeTetAngles.clear();
	TetEdge te;

	struct ChainVert {
		int adjVert0;
		int adjVert1;
		int tet0;
		int tet1;
		float dihed0;
		float dihed1;
		int mark;
	};
	ChainVert cv;
	cv.mark = 0;
	nvidia::Array<ChainVert> chainVerts(mTetVertices.size(), cv);
	nvidia::Array<int> chainVertNrs;

	int mark = 1;

	uint32_t i = 0;
	while (i < edges.size()) {
		SortEdge &e0 = edges[i];
		int edgeNr = (int32_t)mTetEdges.size();
		te.init(e0.i0, e0.i1);
		te.firstTet = (int32_t)mEdgeTetNrs.size();
		te.numTets = 0;
		mark++;
		chainVertNrs.clear();
		do {
			SortEdge &e = edges[i];
			mTetEdgeNrs[uint32_t(6 * e.tetNr + e.edgeNr)] = edgeNr;
			int i2 = -1; 
			int i3 = -1;
			for (uint32_t j = 0; j < 4; j++) {
				int id = mTetIndices[4 * (uint32_t)e.tetNr + j];
				if (id != e0.i0 && id != e0.i1) {
					if (i2 < 0) i2 = id;
					else i3 = id;
				}
			}
			PX_ASSERT(i2 >= 0 && i3 >= 0);

			// dihedral angle at edge
			PxVec3 &p0 = mTetVertices[(uint32_t)e0.i0];
			PxVec3 &p1 = mTetVertices[(uint32_t)e0.i1];
			PxVec3 &p2 = mTetVertices[(uint32_t)i2];
			PxVec3 &p3 = mTetVertices[(uint32_t)i3];
			PxVec3 n2 = (p1-p0).cross(p2-p0); n2.normalize();
			if ((p3-p0).dot(n2) > 0.0f) n2 = -n2;
			PxVec3 n3 = (p1-p0).cross(p3-p0); n3.normalize();
			if ((p2-p0).dot(n3) > 0.0f) n3 = -n3;
			float dot = n2.dot(n3);
			float dihed = PxPi - PxAcos(dot);

			// chain for ordering tets of edge correctly
			ChainVert &cv2 = chainVerts[(uint32_t)i2];
			ChainVert &cv3 = chainVerts[(uint32_t)i3];
			if (cv2.mark != mark) { cv2.adjVert0 = -1; cv2.adjVert1 = -1; cv2.mark = mark; }
			if (cv3.mark != mark) { cv3.adjVert0 = -1; cv3.adjVert1 = -1; cv3.mark = mark; }

			if (cv2.adjVert0 < 0) { cv2.adjVert0 = i3; cv2.tet0 = e.tetNr; cv2.dihed0 = dihed; }
			else { cv2.adjVert1 = i3; cv2.tet1 = e.tetNr; cv2.dihed1 = dihed; }
			if (cv3.adjVert0 < 0) { cv3.adjVert0 = i2; cv3.tet0 = e.tetNr; cv3.dihed0 = dihed; }
			else { cv3.adjVert1 = i2; cv3.tet1 = e.tetNr; cv3.dihed1 = dihed; }

			chainVertNrs.pushBack(i2);
			chainVertNrs.pushBack(i3);
			i++;
		}
		while (i < edges.size() && edges[i] == e0);

		te.numTets = (int32_t)chainVertNrs.size() / 2;
		// find chain start;
		int startVertNr = -1;
		for (uint32_t j = 0; j < chainVertNrs.size(); j++) {
			ChainVert &cv = chainVerts[(uint32_t)chainVertNrs[j]];
			if (cv.adjVert0 < 0 || cv.adjVert1 < 0) {
				startVertNr = chainVertNrs[j];
				break;
			}
		}
		te.onSurface = startVertNr >= 0;
		mTetEdges.pushBack(te);

		int curr = startVertNr;
		if (curr < 0) curr = chainVertNrs[0];
		int prev = -1;

		// collect adjacent tetrahedra
		for (int j = 0; j < te.numTets; j++) {
			ChainVert &cv = chainVerts[(uint32_t)curr];
			int next;
			if (cv.adjVert0 == prev) {
				next = cv.adjVert1;
				mEdgeTetNrs.pushBack(cv.tet1);
				mEdgeTetAngles.pushBack(cv.dihed1);
			}
			else {
				next = cv.adjVert0;
				mEdgeTetNrs.pushBack(cv.tet0);
				mEdgeTetAngles.pushBack(cv.dihed0);
			}
			prev = curr;
			curr = next;
		}
	}
}

// -----------------------------------------------------------------------------
bool CompoundCreator::tetHasColor(int tetNr, int color)
{
	int nr = mTetFirstColor[(uint32_t)tetNr];
	while (nr >= 0) {
		if (mTetColors[(uint32_t)nr].color == color)
			return true;
		nr = mTetColors[(uint32_t)nr].next;
	}
	return false;
}

// -----------------------------------------------------------------------------
bool CompoundCreator::tetAddColor(int tetNr, int color)
{
	if (tetHasColor(tetNr, color))
		return false;
	Color c;
	c.color = color;
	c.next = mTetFirstColor[(uint32_t)tetNr];

	if (mTetColorsFirstEmpty <= 0) {	// new entry
		mTetFirstColor[(uint32_t)tetNr] = (int32_t)mTetColors.size();
		mTetColors.pushBack(c);
	}
	else {		// take from empty list
		int newNr = mTetColorsFirstEmpty;
		mTetFirstColor[(uint32_t)tetNr] = newNr;
		mTetColorsFirstEmpty = mTetColors[(uint32_t)newNr].next;
		mTetColors[(uint32_t)newNr] = c;
	}
	return true;
}

// -----------------------------------------------------------------------------
bool CompoundCreator::tetRemoveColor(int tetNr, int color)
{
	int nr = mTetFirstColor[(uint32_t)tetNr];
	int prev = -1;
	while (nr >= 0) {
		if (mTetColors[(uint32_t)nr].color == color) {
			if (prev < 0) 
				mTetFirstColor[(uint32_t)tetNr] = mTetColors[(uint32_t)nr].next;
			else 
				mTetColors[(uint32_t)prev].next = mTetColors[(uint32_t)nr].next;

			// add to empty list
			mTetColors[(uint32_t)nr].next = mTetColorsFirstEmpty;
			mTetColorsFirstEmpty = nr;
			return true;
		}
		prev = nr;
		nr = mTetColors[(uint32_t)nr].next;
	}
	return false;
}

// -----------------------------------------------------------------------------
int CompoundCreator::tetNumColors(int tetNr)
{
	int num = 0;
	int nr = mTetFirstColor[(uint32_t)tetNr];
	while (nr >= 0) {
		num++;
		nr = mTetColors[(uint32_t)nr].next;
	}
	return num;
}

// -----------------------------------------------------------------------------
void CompoundCreator::deleteColors()
{
	mTetFirstColor.resize(mTetIndices.size()/4, -1);
	mTetColors.clear();
	mTetColorsFirstEmpty = -1;
}

// -----------------------------------------------------------------------------
bool CompoundCreator::tryTet(int tetNr, int color)
{
	if (tetNr < 0)
		return false;

	//if (mTetColors[tetNr] >= 0)
	//	return false;

	if (tetHasColor(tetNr, color))
		return false;

	mTestEdges.clear();
	mAddedTets.clear();

	tetAddColor(tetNr, color);
	mAddedTets.pushBack(tetNr);

	for (int i = 0; i < 6; i++) 
		mTestEdges.pushBack(mTetEdgeNrs[uint32_t(6*tetNr+i)]);

	bool failed = false;

	while (mTestEdges.size() > 0) {
		int edgeNr = mTestEdges[mTestEdges.size()-1];
		mTestEdges.popBack();

		TetEdge &e = mTetEdges[(uint32_t)edgeNr];
		bool anyOtherCol = false;
		float sumAng = 0.0f;
		for (int i = 0; i < e.numTets; i++) {
			int edgeTetNr = mEdgeTetNrs[uint32_t(e.firstTet + i)];
			if (tetHasColor(edgeTetNr, color))
				sumAng += mEdgeTetAngles[uint32_t(e.firstTet + i)];
			else if (tetNumColors(edgeTetNr) > 0)
				anyOtherCol = true;
		}
		if (sumAng < CONVEX_THRESHOLD)
			continue;

//		if (e.onSurface || anyOtherCol) {
		if (e.onSurface) {
			failed = true;
			break;
		}

		for (int i = 0; i < e.numTets; i++) {
			int edgeTetNr = mEdgeTetNrs[uint32_t(e.firstTet + i)];
			if (!tetHasColor(edgeTetNr, color)) {
				tetAddColor(edgeTetNr, color);
				mAddedTets.pushBack(edgeTetNr);
				for (int j = 0; j < 6; j++) 
					mTestEdges.pushBack(mTetEdgeNrs[uint32_t(6*edgeTetNr+j)]);
			}
		}
	}
	if (failed) {
		for (uint32_t i = 0; i < mAddedTets.size(); i++)
			tetRemoveColor(mAddedTets[i], color);
		mAddedTets.clear();
		return false;
	}

	return true;
}

// -----------------------------------------------------------------------------
void CompoundCreator::colorTets()
{
	int numTets = (int32_t)mTetIndices.size() / 4;
	deleteColors();

	int color = 0;
	nvidia::Array<int> edges;
	nvidia::Array<int> faces;

	for (int i = 0; i < numTets; i++) {
		if (tetNumColors(i) > 0)
			continue;

		tetAddColor(i, color);
		faces.clear();
		faces.pushBack(4*i);
		faces.pushBack(4*i+1);
		faces.pushBack(4*i+2);
		faces.pushBack(4*i+3);

		while (faces.size() > 0) {
			int faceNr = faces[faces.size()-1];
			faces.popBack();

			int adjTetNr = mTetNeighbors[(uint32_t)faceNr];

			if (adjTetNr < 0)
				continue;

			//if (tetNumColors(adjTetNr) > 0)
			//	continue;

			if (!tryTet(adjTetNr, color))
				continue;

			for (uint32_t j = 0; j < mAddedTets.size(); j++) {
				int addedTet = mAddedTets[j];
				for (int k = 0; k < 4; k++) {
					int adj = mTetNeighbors[uint32_t(4*addedTet+k)];
					if (adj >= 0 && !tetHasColor(adj, color))
						faces.pushBack(4*addedTet+k);
				}
			}
		}
		color++;
	}
}

// -----------------------------------------------------------------------------
void CompoundCreator::colorsToConvexes()
{
	mGeom.clear();

	int numTets = (int32_t)mTetIndices.size() / 4;
	int numColors = 0;
	for (uint32_t i = 0; i < mTetColors.size(); i++) {
		int color = mTetColors[i].color;
		if (color >= numColors)
			numColors = color+1;
	}

	nvidia::Array<bool> colorVisited((uint32_t)numColors, false);
	nvidia::Array<int> queue;
	nvidia::Array<int> globalToLocal(mTetVertices.size(), -1);

	nvidia::Array<int> tetMarks((uint32_t)numTets, 0);
	nvidia::Array<int> vertMarks(mTetVertices.size(), 0);
	int mark = 1;

	nvidia::Array<int> colorToConvexNr;

	CompoundGeometry::Convex c;

	for (uint32_t i = 0; i < (uint32_t)numTets; i++) {
		int nr = mTetFirstColor[i];
		while (nr >= 0) {
			uint32_t color = (uint32_t)mTetColors[(uint32_t)nr].color;
			nr = mTetColors[(uint32_t)nr].next;

			if (colorVisited[color])
				continue;
			colorVisited[color] = true;

			if ((uint32_t)colorToConvexNr.size() <= color)
				colorToConvexNr.resize(color+1, -1);
			colorToConvexNr[color] = (int32_t)mGeom.convexes.size();

			queue.clear();
			queue.pushBack((int32_t)i);

			mGeom.initConvex(c);
			mark++;
			c.numVerts = 0;

			// flood fill
			while (!queue.empty()) {
				uint32_t tetNr = (uint32_t)queue[queue.size()-1];
				queue.popBack();
				if (tetMarks[tetNr] == mark)
					continue;
				tetMarks[tetNr] = mark;

				for (uint32_t j = 0; j < 4; j++) {
					int adjNr = mTetNeighbors[4*tetNr + j];
					if (adjNr < 0 || !tetHasColor(adjNr, (int32_t)color)) {
						// create new face
						mGeom.indices.pushBack(3);		// face size
						int flags = 0;
						if (adjNr < 0) flags |= CompoundGeometry::FF_OBJECT_SURFACE;
						mGeom.indices.pushBack(flags);

						for (uint32_t k = 0; k < 3; k++) {
							uint32_t id = (uint32_t)mTetIndices[4*tetNr + tetFaceIds[j][k]];
							if (vertMarks[id] != mark) {
								vertMarks[id] = mark;
								globalToLocal[id] = c.numVerts;
								c.numVerts++;
								mGeom.vertices.pushBack(mTetVertices[id]);
							}
							mGeom.indices.pushBack(globalToLocal[id]);
						}
						c.numFaces++;
					}
					if (adjNr >= 0) {
						// add neighbors
						int colNr = mTetFirstColor[(uint32_t)adjNr];
						while (colNr >= 0) {
							int adjColor = mTetColors[(uint32_t)colNr].color;
							colNr = mTetColors[(uint32_t)colNr].next;
							if (adjColor != (int32_t)color) {
								bool isNew = true;
								for (int k = 0; k < c.numNeighbors; k++) {
									if (mGeom.neighbors[uint32_t(c.firstNeighbor+k)] == adjColor) {
										isNew = false;
										break;
									}
								}
								if (isNew) {
									mGeom.neighbors.pushBack(adjColor);
									c.numNeighbors++;
								}
							}
						}
					}
					if (adjNr < 0 || !tetHasColor(adjNr, (int32_t)color) || tetMarks[(uint32_t)adjNr] == mark)
						continue;
					queue.pushBack(adjNr);
				}
			}
			mGeom.convexes.pushBack(c);
		}
	}

	for (uint32_t i = 0; i < mGeom.neighbors.size(); i++) {
		if (mGeom.neighbors[i] >= 0)
			mGeom.neighbors[i] = colorToConvexNr[(uint32_t)mGeom.neighbors[i]];
	}
}

}
}
}
#endif