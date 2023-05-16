#include <algorithm>
#include "rayAccelerator.h"
#include "macros.h"

using namespace std;

BVH::BVHNode::BVHNode(void) {}

void BVH::BVHNode::setAABB(AABB& bbox_) { this->bbox = bbox_; }

void BVH::BVHNode::makeLeaf(unsigned int index_, unsigned int n_objs_) {
	this->leaf = true;
	this->index = index_; 
	this->n_objs = n_objs_; 
}

void BVH::BVHNode::makeNode(unsigned int left_index_) {
	this->leaf = false;
	this->index = left_index_; 
			//this->n_objs = n_objs_; 
}


BVH::BVH(void) {}

int BVH::getNumObjects() { return objects.size(); }


void BVH::Build(vector<Object *> &objs) {

		
			BVHNode *root = new BVHNode();

			Vector min = Vector(FLT_MAX, FLT_MAX, FLT_MAX), max = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			AABB world_bbox = AABB(min, max);

			for (Object* obj : objs) {
				AABB bbox = obj->GetBoundingBox();
				world_bbox.extend(bbox);
				objects.push_back(obj);
			}
			world_bbox.min.x -= EPSILON; world_bbox.min.y -= EPSILON; world_bbox.min.z -= EPSILON;
			world_bbox.max.x += EPSILON; world_bbox.max.y += EPSILON; world_bbox.max.z += EPSILON;
			root->setAABB(world_bbox);
			nodes.push_back(root);
			build_recursive(0, objects.size(), root); // -> root node takes all the 
		}

void BVH::build_recursive(int left_index, int right_index, BVHNode *node) {
	   //PUT YOUR CODE HERE

		if (right_index - left_index <= Threshold) {
			node->makeLeaf(left_index, right_index - left_index);
			return;
		}
		float range_x = 0, range_y = 0, range_z = 0;
		float mid_point = 0;
		int split_index = 0, maxAxis = 0;
		float median = 0;

		//find max Axis Range

		AABB worldbb = node->getAABB();
		range_x = worldbb.max.x - worldbb.min.x;
		range_y = worldbb.max.y - worldbb.min.y;
		range_z = worldbb.max.z - worldbb.min.z;

		range_x > range_y ? (range_x > range_z ? mid_point = range_x/2, maxAxis = 0 : mid_point = range_z/2, maxAxis = 2) 
		: (range_y > range_z ? mid_point = range_y/2, maxAxis = 1 : mid_point = range_z/2, maxAxis = 2) ;

		Comparator cmp;
		cmp.dimension = maxAxis;
		
		//Sort objects by maxAxis
		sort(objects.begin() + left_index, objects.end() + right_index - left_index, cmp);

		//find split index
		for(int i = left_index; i < right_index; i++) {
			if(objects.at(i)->GetBoundingBox().centroid().getAxisValue(maxAxis) > mid_point) {
				split_index = i;
				break;
			}
		}

		//in case the mid point splitting doesnt work, use median
		if (split_index == 0) {
			int size = (right_index - left_index);
			if (size % 2 != 0)
				median = (float)objects.at(size / 2)->GetBoundingBox().centroid().getAxisValue(maxAxis);
			else {
				float m1 = objects.at((size - 1) / 2)->GetBoundingBox().centroid().getAxisValue(maxAxis);
				float m2 = objects.at(size / 2)->GetBoundingBox().centroid().getAxisValue(maxAxis);
				median = (float)(m1 + m2) / 2;
			}
			for (int i = left_index; i < right_index; i++) {
				if (objects.at(i)->GetBoundingBox().centroid().getAxisValue(maxAxis) > median) {
					split_index = i;
					break;
				}
			}
		}

		// Child nodes
		BVHNode* left_child = new BVHNode();
		BVHNode* right_child = new BVHNode();

		// Add to the node vector
		nodes.push_back(left_child);
		nodes.push_back(right_child);

		// Save the left index node on the parent node
		node->makeNode(nodes.size() - 1);

		//Bounding boxes for each new node
		AABB left_box = AABB(Vector(FLT_MAX, FLT_MAX, FLT_MAX), Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX));
		AABB right_box = AABB(Vector(FLT_MAX, FLT_MAX, FLT_MAX), Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX));

		for (int i = left_index; i < split_index; i++) {
			AABB obj_box = objects.at(i)->GetBoundingBox();
			left_box.extend(obj_box);
		}

		for (int i = split_index; i < right_index; i++) {
			AABB obj_box = objects.at(i)->GetBoundingBox();
			right_box.extend(obj_box);
		}

		left_child->setAABB(left_box);
		right_child->setAABB(right_box);

		build_recursive(left_index, split_index, left_child);
		build_recursive(split_index, right_index, right_child);

		//right_index, left_index and split_index refer to the indices in the objects vector
	   // do not confuse with left_nodde_index and right_node_index which refer to indices in the nodes vector. 
	    // node.index can have a index of objects vector or a index of nodes vector
			
		
	}


bool BVH::Traverse(Ray& ray, Object** hit_obj, Vector& hit_point) {
			float tmp;
			float tmin = FLT_MAX;  //contains the closest primitive intersection
			bool hit = false;

			BVHNode* currentNode = nodes[0];

			//PUT YOUR CODE HERE
			
			return(false);
	}

bool BVH::Traverse(Ray& ray) {  //shadow ray with length
			float tmp;

			double length = ray.direction.length(); //distance between light and intersection point
			ray.direction.normalize();

			//PUT YOUR CODE HERE

			return(false);
	}		
