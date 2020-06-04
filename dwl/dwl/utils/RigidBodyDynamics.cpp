#include <dwl/utils/RigidBodyDynamics.h>


namespace dwl
{

namespace rbd
{
RigidBodyDynamics::Math::SpatialVector SpatialVectorZero ( 0., 0., 0., 0., 0., 0.);

std::string coord3dToName(enum Coords3d coord)
{
	const char* Coord3dNames[3] = {"X", "Y", "Z"};

	std::string coord_name(Coord3dNames[coord]);
	return coord_name;
}


std::string coord6dToName(enum Coords6d coord)
{
	const char* Coord6dNames[6] = {"AX", "AY", "AZ", "LX", "LY", "LZ"};

	std::string coord_name(Coord6dNames[coord]);
	return coord_name;
}


Eigen::Vector3d angularPart(Vector6d& vector)
{
	return vector.topRows<3>();
}


Eigen::Vector3d linearPart(Vector6d& vector)
{
	return vector.bottomRows<3>();
}


Eigen::Vector3d translationVector(Eigen::Matrix4d& hom_transform)
{
    return hom_transform.block<3,1>(0,3);
}


Eigen::Matrix3d rotationMatrix(Eigen::MatrixBase<Eigen::Matrix4d>& hom_transform)
{
    return hom_transform.block<3,3>(0,0);
}


void getListOfBodies(BodyID& list_body_id,
					 const RigidBodyDynamics::Model& model)
{
	for (unsigned int it = 0; it < model.mBodies.size(); it++) {
		unsigned int body_id = it;
		std::string body_name = model.GetBodyName(body_id);

		list_body_id[body_name] = body_id;
	}

	// Adding the fixed body in the body list
	for (unsigned int it = 0; it < model.mFixedBodies.size(); it++) {
		unsigned int body_id = it + model.fixed_body_discriminator;
		std::string body_name = model.GetBodyName(body_id);

		list_body_id[body_name] = body_id;
	}
}


void printModelInfo(const RigidBodyDynamics::Model& model)
{
	std::cout << "Degree of freedom overview:" << std::endl;
	std::cout << RigidBodyDynamics::Utils::GetModelDOFOverview(model);

	std::cout << "Body origins overview:" << std::endl;
	RigidBodyDynamics::Model copy_model = model;
	std::cout << RigidBodyDynamics::Utils::GetNamedBodyOriginsOverview(copy_model);

	std::cout << "Model Hierarchy:" << std::endl;
	std::cout << RigidBodyDynamics::Utils::GetModelHierarchy(model);
}


Vector6d convertPointVelocityToSpatialVelocity(Vector6d& velocity,
											   const Eigen::Vector3d& point)
{
	rbd::Vector6d spatial_velocity;
	spatial_velocity.segment<3>(rbd::AX) = angularPart(velocity);
	spatial_velocity.segment<3>(rbd::LX) = linearPart(velocity) +
	math::skewSymmetricMatrixFromVector(point) * angularPart(velocity);

	return spatial_velocity;
}


Vector6d convertPointForceToSpatialForce(Vector6d& force,
										 const Eigen::Vector3d& point)
{
	rbd::Vector6d spatial_force;
	spatial_force.segment<3>(rbd::AX) = angularPart(force) +
			math::skewSymmetricMatrixFromVector(point) * linearPart(force);
	spatial_force.segment<3>(rbd::LX) = linearPart(force);

	return spatial_force;
}


void computePointJacobian(RigidBodyDynamics::Model& model,
						  const RigidBodyDynamics::Math::VectorNd &Q,
						  unsigned int body_id,
						  const RigidBodyDynamics::Math::Vector3d& point_position,
						  RigidBodyDynamics::Math::MatrixNd& G,
						  bool update_kinematics)
{
	using namespace RigidBodyDynamics;
	using namespace RigidBodyDynamics::Math;
	LOG << "-------- " << __func__ << " --------" << std::endl;

	// update the Kinematics if necessary
	if (update_kinematics) {
		UpdateKinematicsCustom(model, &Q, NULL, NULL);
	}

	SpatialTransform point_trans = SpatialTransform (Matrix3d::Identity(),
			CalcBodyToBaseCoordinates(model, Q, body_id, point_position, false));

	assert (G.rows() == 6 && G.cols() == model.qdot_size );

	unsigned int reference_body_id = body_id;

	if (model.IsFixedBodyId(body_id)) {
		unsigned int fbody_id = body_id - model.fixed_body_discriminator;
		reference_body_id = model.mFixedBodies[fbody_id].mMovableParent;
	}

	unsigned int j = reference_body_id;

	// e[j] is set to 1 if joint j contributes to the jacobian that we are
	// computing. For all other joints the column will be zero.
	while (j != 0) {
		unsigned int q_index = model.mJoints[j].q_index;

		if (model.mJoints[j].mDoFCount == 3) {
			G.block<6,3>(0,q_index) = ((point_trans * model.X_base[j].inverse()).toMatrix() * model.multdof3_S[j]);
		} else {
			G.block<6,1>(0,q_index) = point_trans.apply(model.X_base[j].inverse().apply(model.S[j]));
		}

		j = model.lambda[j];
	}
}


rbd::Vector6d computePointVelocity(RigidBodyDynamics::Model& model,
								   const RigidBodyDynamics::Math::VectorNd& Q,
								   const RigidBodyDynamics::Math::VectorNd& QDot,
								   unsigned int body_id,
								   const RigidBodyDynamics::Math::Vector3d point_position,
								   bool update_kinematics)
{
	using namespace RigidBodyDynamics;
	using namespace RigidBodyDynamics::Math;

	LOG << "-------- " << __func__ << " --------" << std::endl;
	assert (model.IsBodyId(body_id));
	assert (model.q_size == Q.size());
	assert (model.qdot_size == QDot.size());

	// Reset the velocity of the root body
	model.v[0].setZero();

	// update the Kinematics with zero acceleration
	if (update_kinematics) {
		UpdateKinematicsCustom(model, &Q, &QDot, NULL);
	}

	unsigned int reference_body_id = body_id;
	Vector3d reference_point = point_position;

	if (model.IsFixedBodyId(body_id)) {
		unsigned int fbody_id = body_id - model.fixed_body_discriminator;
		reference_body_id = model.mFixedBodies[fbody_id].mMovableParent;
		Vector3d base_coords = CalcBodyToBaseCoordinates(model, Q, body_id, point_position, false);
		reference_point = CalcBaseToBodyCoordinates(model, Q, reference_body_id, base_coords, false);
	}

	SpatialVector point_spatial_velocity =
			SpatialTransform(CalcBodyWorldOrientation(model, Q, reference_body_id, false).transpose(),
					reference_point).apply(model.v[reference_body_id]);

	return point_spatial_velocity;
}


rbd::Vector6d computePointAcceleration(RigidBodyDynamics::Model& model,
									   const RigidBodyDynamics::Math::VectorNd& Q,
									   const RigidBodyDynamics::Math::VectorNd& QDot,
									   const RigidBodyDynamics::Math::VectorNd& QDDot,
									   unsigned int body_id,
									   const Eigen::Vector3d point_position,
									   bool update_kinematics)
{
	using namespace RigidBodyDynamics;
	using namespace RigidBodyDynamics::Math;

	LOG << "-------- " << __func__ << " --------" << std::endl;

	// Reset the velocity of the root body
	model.v[0].setZero();
	model.a[0].setZero();

	if (update_kinematics)
		UpdateKinematics(model, Q, QDot, QDDot);

	LOG << std::endl;

	unsigned int reference_body_id = body_id;
	Vector3d reference_point = point_position;

	if (model.IsFixedBodyId(body_id)) {
		unsigned int fbody_id = body_id - model.fixed_body_discriminator;
		reference_body_id = model.mFixedBodies[fbody_id].mMovableParent;
		Vector3d base_coords = CalcBodyToBaseCoordinates(model, Q, body_id, point_position, false);
		reference_point = CalcBaseToBodyCoordinates(model, Q, reference_body_id, base_coords, false);
	}

	SpatialTransform p_X_i (CalcBodyWorldOrientation(model, Q, reference_body_id, false).transpose(), reference_point);

	SpatialVector p_v_i = p_X_i.apply(model.v[reference_body_id]);
	Vector3d a_dash = Vector3d (p_v_i[0], p_v_i[1], p_v_i[2]).cross(Vector3d (p_v_i[3], p_v_i[4], p_v_i[5]));
	SpatialVector p_a_i = p_X_i.apply(model.a[reference_body_id]);

	rbd::Vector6d point_spatial_acceleration;
	point_spatial_acceleration << p_a_i[0], p_a_i[1], p_a_i[2],
								  p_a_i[3] + a_dash[0], p_a_i[4] + a_dash[1], p_a_i[5] + a_dash[2];

	return point_spatial_acceleration;
}


void FloatingBaseInverseDynamics(RigidBodyDynamics::Model& model,
								 const RigidBodyDynamics::Math::VectorNd &Q,
								 const RigidBodyDynamics::Math::VectorNd &QDot,
								 const RigidBodyDynamics::Math::VectorNd &QDDot,
								 RigidBodyDynamics::Math::SpatialVector &base_acc,
								 RigidBodyDynamics::Math::VectorNd &Tau,
								 std::vector<RigidBodyDynamics::Math::SpatialVector> *f_ext)
{//TODO test this algorithm. I'm not sure if it works
	using namespace RigidBodyDynamics;
	using namespace RigidBodyDynamics::Math;

	// Checking if it's a floating-base robot
	bool is_floating_base = false;
	int i = 1;
	while (model.mBodies[i].mIsVirtual) {
		i = model.mu[i][0];
		if (i == 6)
			is_floating_base = true;
	}

	if (is_floating_base) {
		LOG << "-------- " << __func__ << " --------" << std::endl;

		// First pass
		for (unsigned int i = 1; i < 7; i++) {
			unsigned int lambda = model.lambda[i];

			jcalc (model, i, Q, QDot);
			model.X_base[i] = model.X_lambda[i] * model.X_base[lambda];
		}

		for (unsigned int i = 7; i < model.mBodies.size(); i++) {
			unsigned int q_index = model.mJoints[i].q_index;
			unsigned int lambda = model.lambda[i];
			jcalc (model, i, Q, QDot);

			model.X_base[i] = model.X_lambda[i] * model.X_base[lambda];

			model.v[i] = model.X_lambda[i].apply(model.v[lambda]) + model.v_J[i];
			model.c[i] = model.c_J[i] + crossm(model.v[i],model.v_J[i]);

			if (model.mJoints[i].mDoFCount == 3) {
				model.a[i] = model.X_lambda[i].apply(model.a[lambda]) + model.c[i] +
						model.multdof3_S[i] * Vector3d (QDDot[q_index], QDDot[q_index + 1], QDDot[q_index + 2]);
			} else {
				model.a[i] = model.X_lambda[i].apply(model.a[lambda]) + model.c[i] + model.S[i] * QDDot[q_index];
			}

			model.Ic[i] = model.I[i];

			if (!model.mBodies[i].mIsVirtual) {
				model.f[i] = model.I[i] * model.a[i] + crossf(model.v[i],model.I[i] * model.v[i]);
			} else {
				model.f[i].setZero();
			}

			if (f_ext != NULL && (*f_ext)[i] != SpatialVectorZero)
				model.f[i] -= model.X_base[i].toMatrixAdjoint() * (*f_ext)[i];
		}

		// Second pass
		model.Ic[6] = model.I[6];
		model.f[6] = model.I[6] * model.a[6] + crossf(model.v[6],model.I[6] * model.v[6]);
		if (f_ext != NULL && (*f_ext)[6] != SpatialVectorZero)
			model.f[6] -= (*f_ext)[6];

		for (unsigned int i = model.mBodies.size() - 1; i > 6; i--) {
			unsigned int lambda = model.lambda[i];

			model.Ic[lambda] = model.Ic[lambda] + model.X_lambda[i].apply(model.Ic[i]);
			model.f[lambda] = model.f[lambda] + model.X_lambda[i].applyTranspose(model.f[i]);
		}
		base_acc << model.a[6].segment<3>(3) + model.gravity, model.a[6].segment<3>(0);

		// Third pass
		model.a[6] = - model.Ic[6].toMatrix().inverse() * model.f[6];

		for (unsigned int i = 7; i < model.mBodies.size(); i++) {
			unsigned int lambda = model.lambda[i];
			model.a[i] = model.X_lambda[i].apply(model.a[lambda]);

			if (model.mJoints[i].mDoFCount == 3) {
				Tau.block<3,1>(model.mJoints[i].q_index, 0) = model.multdof3_S[i].transpose() *
						(model.Ic[i] * model.a[i] + model.f[i]);
			} else {
				Tau[model.mJoints[i].q_index] = model.S[i].dot(model.Ic[i] * model.a[i] + model.f[i]);
			}
		}
	}
}


void FloatingBaseInverseDynamics(RigidBodyDynamics::Model& model,
								 unsigned int base_dof,
								 const RigidBodyDynamics::Math::VectorNd &Q,
								 const RigidBodyDynamics::Math::VectorNd &QDot,
								 const RigidBodyDynamics::Math::VectorNd &QDDot,
								 RigidBodyDynamics::Math::SpatialVector &base_acc,
								 RigidBodyDynamics::Math::VectorNd &Tau,
								 std::vector<RigidBodyDynamics::Math::SpatialVector> *f_ext)
{//TODO develop this algorithm (probably a general hybrid dynamics?)
	using namespace RigidBodyDynamics;
	using namespace RigidBodyDynamics::Math;

	LOG << "-------- " << __func__ << " --------" << std::endl;

	// First pass
	for (unsigned int i = 1; i < base_dof + 1; i++) {
		unsigned int lambda = model.lambda[i];

		jcalc (model, i, Q, QDot);
		model.X_base[i] = model.X_lambda[i] * model.X_base[lambda];
	}

	model.a[base_dof].set (0., 0., 0., -model.gravity[0], -model.gravity[1], -model.gravity[2]);

	for (unsigned int i = base_dof + 1; i < model.mBodies.size(); i++) {
		unsigned int q_index = model.mJoints[i].q_index;
		unsigned int lambda = model.lambda[i];
		jcalc (model, i, Q, QDot);

		if (lambda != base_dof)
			model.X_base[i] = model.X_lambda[i] * model.X_base[lambda];
		else
			model.X_base[i] = model.X_lambda[i];

		model.v[i] = model.X_lambda[i].apply(model.v[lambda]) + model.v_J[i];
		model.c[i] = model.c_J[i] + crossm(model.v[i],model.v_J[i]);

		if (model.mJoints[i].mDoFCount == 3) {
			model.a[i] = model.X_lambda[i].apply(model.a[lambda]) + model.c[i] +
					model.multdof3_S[i] * Vector3d (QDDot[q_index], QDDot[q_index + 1], QDDot[q_index + 2]);
		} else {
			model.a[i] = model.X_lambda[i].apply(model.a[lambda]) + model.c[i] + model.S[i] * QDDot[q_index];
		}

		model.Ic[i] = model.I[i];

		if (!model.mBodies[i].mIsVirtual) {
			model.f[i] = model.I[i] * model.a[i] + crossf(model.v[i],model.I[i] * model.v[i]);
		} else {
			model.f[i].setZero();
		}

		if (f_ext != NULL && (*f_ext)[i] != SpatialVectorZero)
			model.f[i] -= model.X_base[i].toMatrixAdjoint() * (*f_ext)[i];
	}

	// Second pass
	model.Ic[base_dof] = model.I[base_dof];
	model.f[base_dof] = model.I[base_dof] * model.a[base_dof] +
			crossf(model.v[base_dof],model.I[base_dof] * model.v[base_dof]);
	if (f_ext != NULL && (*f_ext)[base_dof] != SpatialVectorZero)
		model.f[base_dof] -= (*f_ext)[base_dof];

	for (unsigned int i = model.mBodies.size() - 1; i > base_dof; i--) {
		unsigned int lambda = model.lambda[i];

		model.Ic[lambda] = model.Ic[lambda] + model.X_lambda[i].apply(model.Ic[i]);
		model.f[lambda] = model.f[lambda] + model.X_lambda[i].applyTranspose(model.f[i]);
	}

	std::cout << 0 << model.I[0] << std::endl;
	std::cout << 1 << model.I[1] << std::endl;
	std::cout << 2 << model.I[2] << std::endl;
	std::cout << 3 << model.I[3] << std::endl;
	std::cout << 4 << model.I[4] << std::endl;
	// Third pass
	model.a[base_dof] = - model.Ic[base_dof].toMatrix().inverse() * model.f[base_dof];
//	model.a[base_dof] << 0,0,0,0,0, 3.81;//model.a[base_dof](5);
	std::cout << "a0 = " << model.a[base_dof].transpose() << std::endl;
	std::cout << "f0 = " << model.f[base_dof].transpose() << std::endl;
	base_acc << model.a[base_dof].segment<3>(0), model.a[base_dof].segment<3>(3);

	for (unsigned int i = base_dof + 1; i < model.mBodies.size(); i++) {
		unsigned int lambda = model.lambda[i];
		model.a[i] = model.X_lambda[i].apply(model.a[lambda]);

		if (model.mJoints[i].mDoFCount == 3) {
			Tau.block<3,1>(model.mJoints[i].q_index, 0) = model.multdof3_S[i].transpose() *
					(model.Ic[i] * model.a[i] + model.f[i]);
		} else {
			Tau[model.mJoints[i].q_index] = model.S[i].dot(model.Ic[i] * model.a[i] + model.f[i]);
		}
	}
}

} //@namespace rbd
} //@namespace dwl
