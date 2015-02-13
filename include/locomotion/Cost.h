#ifndef DWL_Cost_H
#define DWL_Cost_H

#include <solver/Solver.h>
#include <environment/RewardMap.h>

#include <utils/utils.h>


namespace dwl
{

namespace locomotion
{

/**
 * @class Cost
 * @brief Abstract class for computing the cost of the planning of motion sequence problem. This class is also
 * used for computing terrain and body cost.
 */
class Cost
{
	public:
		/** @brief Constructor function */
		Cost();

		/** @brief Destructor function */
		virtual ~Cost();

		/**
		 * @brief Sets the cost map only for Cost that representing the terrain (or map)
		 * @param std::vector<Cell> Reward map
		 */
		virtual void setCostMap(std::vector<RewardCell> reward_map);

		/**
		 * @brief Abstract method for getting the cost value given a certain state
		 * @param Eigen::VectorXd State value
		 * @return The cost at defined state
		 */
		virtual double get(Eigen::VectorXd state);

		/**
		 * @brief Abstract method for getting the cost value given a certain node
		 * @param AdjacencyMap& adjacency_map Adjacency map required for graph-searching algorithms
		 * @param Eigen::Vector3d robot_state 2D position and yaw orientation of the robot
		 * @param bool terrain_cost Defines if we want to get the terrain cost or body cost
		 */
		virtual void get(AdjacencyMap& adjacency_map, Eigen::Vector3d robot_state, bool terrain_cost);

		/**
		 * @brief Indicates if it was defined a cost map in this class
		 * @return True it was defined a cost map
		 */
		bool isCostMap();

		/**
		 * @brief Gets the name of the cost
		 * @return The name of the cost
		 */
		std::string getName();


	protected:
		/** @brief Name of the cost */
		std::string name_;

		/** @brief Indicates if it is a cost map */
		bool is_cost_map_;
};

} //@namespace locomotion
} //@namespace dwl

#endif
