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

	float min_x = FLT_MAX, min_y = FLT_MAX, min_z = FLT_MAX;
	float max_x = - FLT_MAX, max_y = - FLT_MAX, max_z = - FLT_MAX;

	int split_index = left_index;

	AABB worldbb = node->getAABB();
	float range_x = worldbb.max.x - worldbb.min.x;
	float range_y = worldbb.max.y - worldbb.min.y;
	float range_z = worldbb.max.z - worldbb.min.z;
	float mid_point;

	if (range_x >= range_y) {
		mid_point = range_x;
		max_axis = 0;
	}
	else {
		mid_point = range_y;
		max_axis = 1;
	}

	if (range_z > mid_point) {
		mid_point = range_z;
		max_axis = 2;
	}

	mid_point /= 2.0;
	mid_point += worldbb.min.getAxisValue(max_axis);

	Comparator cmp = Comparator();
	cmp.dimension = max_axis;

	// Sort objects by maxAxis
	std::sort(objects.begin() + left_index, objects.begin() + right_index, cmp);

	//in case the mid point splitting doesnt work(left_index -> ), use median
	if (objects.at(left_index)->getCentroid().getAxisValue(max_axis) > mid_point ||
		objects.at(right_index - 1)->getCentroid().getAxisValue(max_axis) <= mid_point) {
		split_index = (left_index + right_index) / 2;
	}

	else {
		// Find split index
		for (split_index = left_index; split_index < right_index; split_index++) {
			if (objects.at(split_index)->getCentroid().getAxisValue(max_axis) > mid_point) {
				break;
			}
		}
	}
	
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

	// Child nodes
	BVHNode* left_child = new BVHNode();
	BVHNode* right_child = new BVHNode();

	// Save the left index node on the parent node
	node->makeNode(nodes.size());

	left_child->setAABB(left_box);
	right_child->setAABB(right_box);

	// Add to the node vector
	nodes.push_back(left_child);
	nodes.push_back(right_child);

	build_recursive(left_index, split_index, left_child);
	build_recursive(split_index, right_index, right_child);
			
}

Object* BVH::findIntersection(Ray& ray, BVHNode* current_node, float* t_ret) {
	float t;
	Object* closest_obj = nullptr;

	if (!current_node->getAABB().intercepts(ray, t)) {
		return nullptr;
	}

	while (true) {
		
		if (current_node->isLeaf()) { //find the closest hit with the objects of the node

			int base_index = current_node->getIndex(), size = current_node->getNObjs();

			for (int i = base_index; i < base_index + size; i++) {
				bool inter = objects[i]->intercepts(ray, t);

				if (inter && t < *t_ret) {
					*t_ret = t;
					closest_obj = objects[i];
				}
			}
		}
		else {
			
			float t1 = FLT_MAX, t2 = FLT_MAX;
			BVHNode* child1 = nodes[current_node->getIndex()];
			BVHNode* child2 = nodes[current_node->getIndex() + 1];

			bool c1 = child1->getAABB().intercepts(ray, t1);// Test node�s children
			bool c2 = child2->getAABB().intercepts(ray, t2);// Test node�s children

			if (child1->getAABB().isInside(ray.origin)) t1 = 0;
			if (child2->getAABB().isInside(ray.origin)) t2 = 0;

			StackItem* s = nullptr;

			if (c1 && c2) {
				if (t2 < t1) {
					current_node = child2;
					s = new StackItem(child1, t1);
				}
				else {
					current_node = child1;
					s = new StackItem(child2, t2);
				}

				hit_stack.push(*s);
				continue;
			}
			else if (c1) {
				current_node = child1;
				continue;
			}
			else if (c2) {
				current_node = child2;
				continue;
			}
		}

		//Get from stack
		while (true) {
			if (hit_stack.empty()){
				return closest_obj;
			}
			else {
				StackItem s = hit_stack.top();
				hit_stack.pop();

				if (s.t < *t_ret) {
					current_node = s.ptr;
					break;
				}
			}
		}
	}
}


bool BVH::Traverse(Ray& ray, Object** hit_obj, Vector& hit_point) {
	float tmin = FLT_MAX;  //contains the closest primitive intersection
	bool hit = false; 

	*hit_obj = findIntersection(ray, nodes[0], &tmin);
	
	if (*hit_obj == nullptr) {
		return false;
	}

	hit_point = ray.origin + ray.direction * tmin;
	return true;
}

bool BVH::findIntersection(Ray& ray, BVHNode* current_node) {
	float t, max_t = ray.direction.length();
	Object* closest_obj = nullptr;

	if (!current_node->getAABB().intercepts(ray, t)) {
		return false;
	}

	while (true) {

		if (current_node->isLeaf()) { //find the closest hit with the objects of the node
			int base_index = current_node->getIndex(), size = current_node->getNObjs();

			for (int i = base_index; i < base_index + size; i++) {
				if (objects[i]->intercepts(ray, t) && t < max_t) {
					return true;
				}
			}
		}
		else {
			float t1 = FLT_MAX, t2 = FLT_MAX;
			BVHNode* child1 = nodes[current_node->getIndex()];
			BVHNode* child2 = nodes[current_node->getIndex() + 1];

			bool c1 = child1->getAABB().intercepts(ray, t1);// Test node�s children
			bool c2 = child2->getAABB().intercepts(ray, t2);// Test node�s children

			StackItem* s = nullptr;

			if (c1 && c2) {
				current_node = child1;

				s = new StackItem(child2, t2);
				hit_stack.push(*s);

				continue;
			}
			else if (c1) {
				current_node = child1;
				continue;
			}
			else if (c2) {
				current_node = child2;
				continue;
			}
		}

		//Get from stack
		
		if (hit_stack.empty()) {
			return false;
		}
		else {
			StackItem s = hit_stack.top();
			hit_stack.pop();
			current_node = s.ptr;
		}

	}
}

bool BVH::Traverse(Ray& ray) {  //shadow ray with length
	return findIntersection(ray, nodes[0]);
}		
