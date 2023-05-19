#include <algorithm>
#include "rayAccelerator.h"
#include "macros.h"

using namespace std;

int max_axis = 0;

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
	if (right_index - left_index <= Threshold) {
		node->makeLeaf(left_index, right_index - left_index);
		return;
	}
	float range_x = 0, range_y = 0, range_z = 0;
	float mid_point = 0;
	int split_index = 0;
	float median = 0;

	//find max Axis Range

	AABB worldbb = node->getAABB();
	range_x = worldbb.max.x - worldbb.min.x;
	range_y = worldbb.max.y - worldbb.min.y;
	range_z = worldbb.max.z - worldbb.min.z;

	if (range_x > range_y && range_x > range_z) {
		mid_point = range_x / 2;
		max_axis = 0;
	}
	else if (range_y > range_z) {
		mid_point = range_y / 2;
		max_axis = 1;
	}
	else {
		mid_point = range_z / 2;
		max_axis = 2;
	}

	// range_x > range_y ? (range_x > range_z ? mid_point = range_x/2, max_axis = 0 : mid_point = range_z/2, max_axis = 2) 
	// : (range_y > range_z ? mid_point = range_y/2, max_axis = 1 : mid_point = range_z/2, max_axis = 2);

	Comparator cmp = Comparator();
	cmp.dimension = max_axis;
		
	//Sort objects by maxAxis
	std::sort(objects.begin() + left_index, objects.end() + right_index - left_index, cmp);

	//find split index
	for (int i = left_index; i < right_index; i++) {
		if (objects.at(i)->GetBoundingBox().centroid().getAxisValue(max_axis) > mid_point) {
			split_index = i;
			break;
		}
	}

	//in case the mid point splitting doesnt work, use median
	if (split_index == 0) {
		int size = (right_index - left_index);
		if (size % 2 != 0)
			median = (float)objects.at(size / 2)->GetBoundingBox().centroid().getAxisValue(max_axis);
		else {
			float m1 = objects.at((size - 1) / 2)->GetBoundingBox().centroid().getAxisValue(max_axis);
			float m2 = objects.at(size / 2)->GetBoundingBox().centroid().getAxisValue(max_axis);
			median = (float)(m1 + m2) / 2;
		}
		for (int i = left_index; i < right_index; i++) {
			if (objects.at(i)->GetBoundingBox().centroid().getAxisValue(max_axis) > median) {
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

Object* BVH::findIntersection(Ray& ray, BVHNode* current_node, float* t_ret) {
	float t;
	Object* closest_obj = nullptr;

	if (!current_node->getAABB().intercepts(ray, t)) {
		return nullptr;
	}

	while (true) {

		if (current_node->isLeaf()) { //find the closest hit with the objects of the node
			for (int i = current_node->getIndex(); i < current_node->getNObjs(); i++) {
				if (objects[i]->intercepts(ray, t) && t > 0 && t < *t_ret) {
					*t_ret = t;
					closest_obj = objects[i];
				}
			}
		}
		else {
			float t1 = FLT_MAX, t2 = FLT_MAX;
			BVHNode* child1 = nodes[current_node->getIndex()];
			BVHNode* child2 = nodes[current_node->getIndex() + 1];
			Object* c1 = findIntersection(ray, child1, &t1); // Test node’s children
			Object* c2 = findIntersection(ray, child2, &t2); // Test node’s children
			StackItem* s = nullptr;
			if (t1 > t2) {
				*t_ret = t2;
				current_node = child2;
				if (c1 != nullptr)
					s = new StackItem(child1, t1);
			}
			else if (t1 < t2) {
				*t_ret = t1;
				current_node = child1;
				if (c2 != nullptr)
					s = new StackItem(child2, t2);
			}
			if (s != nullptr) // Test if there were 2 hits (otherwise
				hit_stack.push(*s);
		}

		if (!hit_stack.empty())
			return closest_obj;

		while (!hit_stack.empty()) {
			StackItem s = hit_stack.top();
			if (s.t < *t_ret)
				current_node = s.ptr;
			hit_stack.pop();
		}
	}
}


bool BVH::Traverse(Ray& ray, Object** hit_obj, Vector& hit_point) {
	float tmin = FLT_MAX;  //contains the closest primitive intersection
	bool hit = false; 

	*hit_obj = findIntersection(ray, nodes[0], &tmin);
	if (*hit_obj == nullptr)
		return false;

	hit_point = ray.origin + ray.direction * tmin;
	return true;
}

bool BVH::findIntersection(Ray& ray, BVHNode* current_node) {
	float t;
	Object* closest_obj = nullptr;

	if (!current_node->getAABB().intercepts(ray, t)) {
		return nullptr;
	}

	while (true) {

		if (current_node->isLeaf()) { //find the closest hit with the objects of the node
			for (int i = current_node->getIndex(); i < current_node->getNObjs(); i++) {
				if (objects[i]->intercepts(ray, t)) {
					return true;
				}
			}
		}
		else {
			float t1 = FLT_MAX, t2 = FLT_MAX;
			BVHNode* child1 = nodes[current_node->getIndex()];
			BVHNode* child2 = nodes[current_node->getIndex() + 1];
			Object* c1 = findIntersection(ray, child1, &t1); // Test node’s children
			Object* c2 = findIntersection(ray, child2, &t2); // Test node’s children
			StackItem* s = nullptr;
			if (t1 < FLT_MAX && t2 < FLT_MAX) {
				current_node = child1;
				s = new StackItem(child2, t2);
			}
			else if (t1 < FLT_MAX) {
				current_node = child1;
			}
			else if (t2 < FLT_MAX) {
				current_node = child2;
			}
		}

		if (!hit_stack.empty())
			return false;

		StackItem s = hit_stack.top();
		current_node = s.ptr;
		hit_stack.pop();
	}
}

bool BVH::Traverse(Ray& ray) {  //shadow ray with length
	return findIntersection(ray, nodes[0]);
}		
