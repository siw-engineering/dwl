#include <dwl/utils/URDF.h>
#include <fstream>
#include <memory>

namespace dwl
{

namespace urdf_model
{

std::string fileToXml(const std::string& filename)
{
	// Reading the file
	std::ifstream model_file(filename.c_str());
	if (!model_file) {
		std::cerr << "Error opening file '" << filename << "'." << std::endl;
		abort();
	}

	// Reserving memory for the contents of the file
	std::string model_xml_string;
	model_file.seekg(0, std::ios::end);
	model_xml_string.reserve(model_file.tellg());
	model_file.seekg(0, std::ios::beg);
	model_xml_string.assign((std::istreambuf_iterator<char>(model_file)),
			std::istreambuf_iterator<char>());
	model_file.close();

	return model_xml_string;
}


void getJointNames(JointID& joints,
				   const std::string& urdf_model,
				   enum JointType type)
{
	// Parsing the URDF-XML
	std::shared_ptr<urdf::ModelInterface> model = urdf::parseURDF(urdf_model);

	std::stack<std::shared_ptr<urdf::Link> > link_stack;
	std::stack<int> branch_index_stack;

	// Adding the bodies in a depth-first order of the model tree
	std::map<std::string, std::shared_ptr<urdf::Link> > link_map = model->links_;
	link_stack.push(link_map[model->getRoot()->name]);

	if (link_stack.top()->child_joints.size() > 0) {
		branch_index_stack.push(0);
	}

	unsigned int joint_idx = 0;
	while (link_stack.size() > 0) {
		std::shared_ptr<urdf::Link> current_link = link_stack.top();
		unsigned int branch_idx = branch_index_stack.top();

		if (branch_idx < current_link->child_joints.size()) {
			std::shared_ptr<urdf::Joint> current_joint =
					current_link->child_joints[branch_idx];

			// Incrementing branch index
			branch_index_stack.pop();
			branch_index_stack.push(branch_idx + 1);

			link_stack.push(link_map[current_joint->child_link_name]);
			branch_index_stack.push(0);

			// Searching joints names
			if (type == free) {
				if (current_joint->type == urdf::Joint::FLOATING ||
						current_joint->type == urdf::Joint::PRISMATIC ||
						current_joint->type == urdf::Joint::REVOLUTE ||
						current_joint->type == urdf::Joint::CONTINUOUS) {
					joints[current_joint->name] = joint_idx;
					joint_idx++;
				}
			} else if (type == fixed) {
				if (current_joint->type == urdf::Joint::FIXED) {
					joints[current_joint->name] = joint_idx;
					joint_idx++;
				}
			} else if (type == floating) {
				if (current_joint->type == urdf::Joint::FLOATING) {
					joints[current_joint->name] = joint_idx;
					joint_idx++;
				}

				if (current_joint->type == urdf::Joint::PRISMATIC ||
						current_joint->type == urdf::Joint::REVOLUTE ||
						current_joint->type == urdf::Joint::CONTINUOUS) {
					if (current_joint->limits->effort == 0) {
						joints[current_joint->name] = joint_idx;
						joint_idx++;
					}
				}
			} else { // all joints
				if (current_joint->type == urdf::Joint::FIXED)
					joints[current_joint->name] =
							std::numeric_limits<unsigned int>::max();
				else {
					joints[current_joint->name] = joint_idx;
					joint_idx++;
				}
			}
		} else {
			link_stack.pop();
			branch_index_stack.pop();
		}
	}
}


void getEndEffectors(LinkID& end_effectors,
					 const std::string& urdf_model)
{
	// Parsing the URDF-XML
	std::shared_ptr<urdf::ModelInterface> model = urdf::parseURDF(urdf_model);

	// Getting the fixed joint names
	JointID fixed_joints;
	getJointNames(fixed_joints, urdf_model, fixed);

	// Getting the world, root, parent and child links
	std::shared_ptr<urdf::Link> world_link = model->links_[model->getRoot()->name];
	std::shared_ptr<urdf::Link> root_link = world_link->child_links[0];

	// Searching the end-effector joints
	unsigned int end_effector_idx = 0;
	for (urdf_model::JointID::iterator jnt_it = fixed_joints.begin();
			jnt_it != fixed_joints.end(); jnt_it++) {
		std::string joint_name = jnt_it->first;
		std::shared_ptr<urdf::Joint> current_joint = model->joints_[joint_name];

		// Getting the parent and child link of the current fixed joint
		std::shared_ptr<urdf::Link> parent_link =
				model->links_[current_joint->parent_link_name];
		std::shared_ptr<urdf::Link> child_link =
				model->links_[current_joint->child_link_name];

		// Checking if it's an end-effector
		unsigned int num_childs = child_link->child_joints.size();
		while (parent_link->name != root_link->name && num_childs == 0) {
			std::shared_ptr<urdf::Joint> parent_joint =
					model->joints_[parent_link->parent_joint->name];
			if (parent_joint->type == urdf::Joint::PRISMATIC ||
					parent_joint->type == urdf::Joint::REVOLUTE) {
				end_effectors[current_joint->child_link_name] = end_effector_idx;
				end_effector_idx++;
				break;
			}

			parent_link = parent_link->getParent();
		}
	}
}


void getJointLimits(JointLimits& joint_limits,
					const std::string& urdf_model)
{
	// Parsing the URDF-XML
	std::shared_ptr<urdf::ModelInterface> model = urdf::parseURDF(urdf_model);

	// Getting the free joint names
	JointID free_joints;
	getJointNames(free_joints, urdf_model, free);

	// Computing the number of actuated joints
	unsigned num_joints = 0;
	for (urdf_model::JointID::iterator jnt_it = free_joints.begin();
			jnt_it != free_joints.end(); jnt_it++) {
		std::string joint_name = jnt_it->first;
		std::shared_ptr<urdf::Joint> current_joint = model->joints_[joint_name];

		if (current_joint->type != urdf::Joint::FLOATING)
			if (current_joint->limits->effort != 0) // Virtual floating-base joints
				num_joints++;
	}

	// Searching the joint limits
	for (urdf_model::JointID::iterator jnt_it = free_joints.begin();
			jnt_it != free_joints.end(); jnt_it++) {
		std::string joint_name = jnt_it->first;
		std::shared_ptr<urdf::Joint> current_joint = model->joints_[joint_name];

		if (current_joint->type != urdf::Joint::FLOATING) {
			if (current_joint->limits->effort != 0) { // Virtual floating-base joints
				joint_limits[joint_name].lower = current_joint->limits->lower;
				joint_limits[joint_name].upper = current_joint->limits->upper;
				joint_limits[joint_name].velocity = current_joint->limits->velocity;
				joint_limits[joint_name].effort = current_joint->limits->effort;
			}
		}
	}
}


void getJointAxis(JointAxis& joints,
				  const std::string& urdf_model,
				  enum JointType type)
{
	// Parsing the URDF-XML
	std::shared_ptr<urdf::ModelInterface> model = urdf::parseURDF(urdf_model);

	// Getting the joint names
	JointID joint_ids;
	getJointNames(joint_ids, urdf_model, type);

	for (urdf_model::JointID::iterator jnt_it = joint_ids.begin();
			jnt_it != joint_ids.end(); jnt_it++) {
		std::string joint_name = jnt_it->first;
		std::shared_ptr<urdf::Joint> current_joint = model->joints_[joint_name];

		if (current_joint->type != urdf::Joint::FLOATING ||
				current_joint->type != urdf::Joint::FIXED)
			joints[joint_name] << current_joint->axis.x,
								  current_joint->axis.y,
								  current_joint->axis.z;
		else
			joints[joint_name] = Eigen::Vector3d::Zero();
	}
}


void getFloatingBaseJointMotion(JointID& joints,
								const std::string& urdf_model)
{
	// Parsing the URDF-XML
	std::shared_ptr<urdf::ModelInterface> model = urdf::parseURDF(urdf_model);

	// Getting the free joint names
	JointAxis joint_axis;
	getJointAxis(joint_axis, urdf_model, floating);

	for (urdf_model::JointAxis::iterator jnt_it = joint_axis.begin();
			jnt_it != joint_axis.end(); jnt_it++) {
		std::string joint_name = jnt_it->first;
		std::shared_ptr<urdf::Joint> current_joint = model->joints_[joint_name];

		// Getting the kind of motion (prismatic or rotation)
		if (current_joint->type == urdf::Joint::FLOATING)
			joints[joint_name] = FULL;
		else if (current_joint->type == urdf::Joint::REVOLUTE ||
				current_joint->type == urdf::Joint::CONTINUOUS) {
			// Getting the axis of movements
			if (current_joint->axis.x != 0) {
				joints[joint_name] = RX;
				break;
			}
			if (current_joint->axis.y != 0) {
				joints[joint_name] = RY;
				break;
			}
			if (current_joint->axis.z != 0) {
				joints[joint_name] = RZ;
				break;
			}
		} else if (current_joint->type == urdf::Joint::PRISMATIC) {
			// Getting the axis of movements
			if (current_joint->axis.x != 0) {
				joints[joint_name] = TX;
				break;
			}
			if (current_joint->axis.y != 0) {
				joints[joint_name] = TY;
				break;
			}
			if (current_joint->axis.z != 0) {
				joints[joint_name] = TZ;
				break;
			}
		}
	}
}

} //@namespace urdf_model
} //@namespace dwl
