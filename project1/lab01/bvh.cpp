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

	//printf("Start left %d / right %d\n", left_index, right_index);

	if (right_index - left_index <= Threshold) {
		node->makeLeaf(left_index, right_index - left_index);
		
		//printf("Leaf %d %d\n", left_index, right_index);
		return;
	}

	//printf("HERE1\n");

	float range_x = 0, range_y = 0, range_z = 0;
	float mid_point = 0;
	int split_index = 0;

	//find max Axis Range

	AABB worldbb = node->getAABB();
	range_x = worldbb.max.x - worldbb.min.x;
	range_y = worldbb.max.y - worldbb.min.y;
	range_z = worldbb.max.z - worldbb.min.z;

	//printf("start ranges %f %f %f\n", range_x, range_y, range_z);

	if (range_x > range_y) {
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

	//printf("mid %f\n", mid_point);

	Comparator cmp = Comparator();
	cmp.dimension = max_axis;

	//Sort objects by maxAxis
	std::sort(objects.begin() + left_index, objects.begin() + right_index, cmp);

	//printf("HERE2\n");

	//find split index
	for (int i = left_index; i < right_index; i++) {
		//printf("%f ", objects.at(i)->GetBoundingBox().centroid().getAxisValue(max_axis));
		if (objects.at(i)->GetBoundingBox().centroid().getAxisValue(max_axis) > mid_point) {
			split_index = i;
			break;
		}
	}

	//printf("\n");

	//printf("Start split %d\n", split_index);

	//in case the mid point splitting doesnt work(left_index -> ), use median
	if (split_index == left_index || split_index == 0) {
		
		int size = (right_index - left_index);
		split_index = left_index + size / 2;
	}

	//printf("split %d\n", split_index);

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

	//printf("HERE3\n");

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

	//printf("left %d / right %d\n", left_index, split_index);

	build_recursive(left_index, split_index, left_child);
	//printf("HERE5-----\n");
	build_recursive(split_index, right_index, right_child);
	//printf("HERE6------\n");

	//right_index, left_index and split_index refer to the indices in the objects vector
	// do not confuse with left_nodde_index and right_node_index which refer to indices in the nodes vector. 
	// node.index can have a index of objects vector or a index of nodes vector
			
}

Object* BVH::findIntersection(Ray& ray, BVHNode* current_node, float* t_ret) {
	float t;
	Object* closest_obj = nullptr;

	if (!current_node->getAABB().intercepts(ray, t)) {
		//printf("Hello\n");
		return nullptr;
	}

	while (true) {

		if (current_node->isLeaf()) { //find the closest hit with the objects of the node
			//printf("Leaf %d %d\n", current_node->getIndex(), current_node->getIndex() + current_node->getNObjs());

			int base_index = current_node->getIndex(), size = current_node->getNObjs();

			for (int i = base_index; i < base_index + size; i++) {
				bool inter = objects[i]->intercepts(ray, t);
				/*if (inter)
					printf("true %f\n", t);*/

				if (inter && t > 0 && t < *t_ret) {
					*t_ret = t;
					closest_obj = objects[i];
					//printf("Object hit %d\n", i);
				}
			}
		}
		else {
			//printf("Middle -- %d\n", current_node->getIndex());
			float t1 = FLT_MAX, t2 = FLT_MAX;
			BVHNode* child1 = nodes[current_node->getIndex()];
			BVHNode* child2 = nodes[current_node->getIndex() + 1];

			bool c1 = child1->getAABB().intercepts(ray, t1);// Test node’s children
			bool c2 = child2->getAABB().intercepts(ray, t2);// Test node’s children

			if (child1->getAABB().isInside(ray.origin)) t1 = 0;
			if (child2->getAABB().isInside(ray.origin)) t2 = 0;

			StackItem* s = nullptr;

			if (c1 && c2) {
				if (t2 < t1) {
					//printf("Case1\n");
					current_node = child2;
					s = new StackItem(child1, t1);
				}
				else {
					//printf("Case2\n");
					current_node = child1;
					s = new StackItem(child2, t2);
				}

				hit_stack.push(*s);
				continue;
			}
			else if (c1) {
				//printf("Case3\n");
				current_node = child1;
				continue;
			}
			else if (c2) {
				//printf("Case4\n");
				current_node = child2;
				continue;
			}

			//printf("Case5\n");
		}

		//Get from stack
		while (true) {
			if (hit_stack.empty()){
				return closest_obj;
			}
			else {
				StackItem s = hit_stack.top();
				hit_stack.pop();
				
				//printf("-------------------Pop! %f %f\n", s.t, *t_ret);
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
	//printf("End Intersection\n");
	if (*hit_obj == nullptr) {
		return false;
	}

	hit_point = ray.origin + ray.direction * tmin;
	return true;
}

bool BVH::findIntersection(Ray& ray, BVHNode* current_node) {
	float t;
	Object* closest_obj = nullptr;


	//----------------TODO: verify lenght do shadow ray

	if (!current_node->getAABB().intercepts(ray, t)) {
		return false;
	}

	while (true) {

		if (current_node->isLeaf()) { //find the closest hit with the objects of the node
			int base_index = current_node->getIndex(), size = current_node->getNObjs();

			for (int i = base_index; i < base_index + size; i++) {
				if (objects[i]->intercepts(ray, t)) {
					return true;
				}
			}
		}
		else {
			float t1 = FLT_MAX, t2 = FLT_MAX;
			BVHNode* child1 = nodes[current_node->getIndex()];
			BVHNode* child2 = nodes[current_node->getIndex() + 1];

			bool c1 = child1->getAABB().intercepts(ray, t1);// Test node’s children
			bool c2 = child2->getAABB().intercepts(ray, t2);// Test node’s children

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
