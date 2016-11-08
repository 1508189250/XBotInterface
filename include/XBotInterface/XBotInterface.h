/*
 * Copyright (C) 2016 IIT-ADVR
 * Author: Arturo Laurenzi, Luca Muratore
 * email:  arturo.laurenzi@iit.it, luca.muratore@iit.it
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>
*/

#ifndef __I_XBOT_INTERFACE_H__
#define __I_XBOT_INTERFACE_H__

#include <vector>
#include <map>
#include <memory>
#include <algorithm>

#include <yaml-cpp/yaml.h>
#include <XBotCoreModel.h>
#include <XBotInterface/KinematicChain.h>

#define LIB_MIDDLE_PATH "/build/install/lib/"

namespace XBot {

    class XBotInterface {
    
    public:

        
        typedef std::shared_ptr<XBotInterface> Ptr;
        
        // Constructor, copy constructor, copy assignment, virtual destructor, init function
        
        XBotInterface();
        XBotInterface(const XBotInterface& other);
        XBotInterface& operator= (const XBotInterface& rhs);
        virtual ~XBotInterface();
        
        bool init(const std::string& path_to_cfg);


        // URDF, SRDF getters

        /**
         * @brief Getter for the robot URDF model corresponding to the URDF xml file
         * specified in the YAML config file.
         * 
         * @return A reference to const urdf::ModelInterface
         */
        const urdf::ModelInterface& getUrdf() const;
        
        /**
         * @brief Getter fot the robot SRDF model corresponding to the SRDF xml file
         * specified in the YAML config file.
         * 
         * @return A reference to const srdf_advd::Model
         */
        const srdf_advr::Model& getSrdf() const;
        
        /**
         * @brief Returns the robot URDF xml as a string. The URDF file is specified
         * inside the YAML config file.
         * 
         * @return A const reference to the required string.
         */
        const std::string& getUrdfString() const;
        
        /**
         * @brief Returns the robot SRDF xml as a string. The SRDF file is specified
         * inside the YAML config file.
         * 
         * @return A const reference to the required string.
         */
        const std::string& getSrdfString() const;
        
        
        // Group states
        
        /**
        * @brief Gets the robot joints group state configuration as specified in the robot SRDF.
        * 
        * 
        * @param state_name The name of the requested group state.
        * @param q The chain joint configuration as Eigen vector of joint positions,
        *          This is an output parameter; any contents present in the vector will be overwritten in this function.
        * @return True if the state_name exists in the SRDF, false otherwise.
        */
        bool getRobotState(const std::string& state_name, Eigen::VectorXd& q) const;
        
        /**
        * @brief Gets the robot joints group state configuration as specified in the robot SRDF.
        * 
        * @param state_name The name of the requested group state.
        * @param q The robot joint configuration as a map with key representing the joint ID (i.e. numerical name of the joint) and value representing joint positions. This is an output parameter; it will not be cleared before being filled.
        * @return True if the state_name exists in the SRDF, false otherwise.
        */
        bool getRobotState(const std::string& state_name, std::map<int, double>& q) const;
        
        /**
        * @brief Gets the robot joints group state configuration as specified in the robot SRDF.
        * 
        * @param state_name The name of the requested group state.
        * @param q The robot joint configuration as a map with key representing the joint name and value representing joint positions. This is an output parameter; it will not be cleared before being filled.
        * @return True if the state_name exists in the SRDF, false otherwise.
        */
        bool getRobotState(const std::string& state_name, std::map<std::string, double>& q) const;


        // Kinematic chains

        /**
         * @brief Return a vector of available chain names as strings.
         * 
         * @return A vector of available chain names.
         */
        std::vector<std::string> getChainNames() const;
        
        /**
         * @brief A method for determining if a chain with name "chain_name" is defined
         * inside the interface.
         * 
         * @param chain_name The name of the chain whose existence has to be checked.
         * @return True if the required chain exists.
         */
        bool hasChain(const std::string& chain_name) const;
        
        /**
         * @brief Returns the number of legs defined inside the interface. This equals
         * the number of elements of the "legs" group inside the SRDF which was provided in the 
         * YAML config file.
         * 
         * @return The number of defined legs.
         */
        int legs() const;
        
        /**
         * @brief Returns the number of arms defined inside the interface. This equals
         * the number of elements of the "arms" group inside the SRDF which was provided in the 
         * YAML config file.
         * 
         * @return The number of defined arms.
         */        
        int arms() const;

        // Joints

        /**
         * @brief Returns a vector of enabled joint names.
         * 
         * @return A const reference to the vector of enabled joint names.
         */
        const std::vector<std::string>& getEnabledJointNames() const;
        
        /**
         * @brief Returns a vector of enabled joint IDs.
         * 
         * @return A const reference to the vector of enabled joint IDs.
         */
        const std::vector<int>& getEnabledJointId() const;
        
        /**
         * @brief Checks that a joint with name "joint_name" is defined as an enabled
         * joint inside the interface.
         * 
         * @param joint_name The name of the joint we want to check existence for.
         * @return True if the required joint is defined and enabled.
         */
        bool hasJoint(const std::string& joint_name) const;
        
        /**
         * @brief Getter for the number of enabled joints.
         * 
         * @return The number of enabled joints.
         */
        int getJointNum() const;

        /**
         * @brief Gets the joint with name "joint_name" if it is defined as enabled.
         * Otherwise, a null pointer is returned and an error is printed to screen.
         * 
         * @param joint_name The name of the required joint.
         * @return A const shared pointer to the required joint. A null pointer is returned if
         * joint_name is not defined as enabled.
         */
        XBot::Joint::ConstPtr getJointByName(const std::string& joint_name) const;
        
        /**
         * @brief Gets the joint with ID "joint_id" if it is defined as enabled.
         * Otherwise, a null pointer is returned and an error is printed to screen.
         * Joint IDs are defined inside a YAML file which is specified in the config
         * file which is used to initialize the interface.
         * 
         * @param joint_id The ID of the required joint.
         * @return A const shared pointer to the required joint. A null pointer is returned if
         * a joint with the requested ID is not defined as enabled.
         */
        XBot::Joint::ConstPtr getJointByID(int joint_id) const;
        
        /**
         * @brief Gets the joint with the required Eigen index. This means that the idx-th
         * entry of all Eigen vectors which are taken as arguments by XBotInterface methods refers
         * to the returned joint.
         * 
         * @param idx The Eigen idx of the required joint. Valid indices span from 0 to getJointNum()-1.
         * @return A const shared pointer to the required joint. A null pointer is returned if
         * idx is not a valid index.
         */
        XBot::Joint::ConstPtr getJointByDofIndex(int idx) const;
        
        /**
         * @brief Gets the Eigen ID of the joint with name "joint_name". This means that the i-th
         * entry of all Eigen vectors which are taken as arguments by XBotInterface methods refers
         * to the joint namd "joint_name".
         * 
         * @param joint_name The name of the required joint.
         * @return The Eigen index of the required joint.
         */
        int getDofIndex(const std::string& joint_name) const;
        
        /**
         * @brief Gets the Eigen ID of the joint with ID "joint_id". This means that the i-th
         * entry of all Eigen vectors which are taken as arguments by XBotInterface methods refers
         * to the joint with the given ID.
         * 
         * @param joint_id The ID of the required joint.
         * @return The Eigen index of the required joint.
         */
        int getDofIndex(int joint_id) const;
        
        /**
         * @brief Gets dof indices of all joints inside chain "chain_name".
         * 
         * @param chain_name The name of the requested chain.
         * @param ids The output vector of dof indices.
         * @return True if chain_name is a valid chain name.
         */
        bool getDofIndex(const std::string& chain_name, std::vector<int>& ids) const;


        // Force-torque sensors

        std::map< std::string, ForceTorqueSensor::ConstPtr > getForceTorque();
        ForceTorqueSensor::ConstPtr getForceTorque(const std::string& parent_link_name) const;
        
        // IMU sensors

        std::map< std::string, ImuSensor::ConstPtr > getImu();
        ImuSensor::ConstPtr getImu(const std::string& parent_link_name) const;



        // Getters for RX


        bool getJointPosition(Eigen::VectorXd& q) const;
        bool getMotorPosition(Eigen::VectorXd& q) const;
        bool getJointVelocity(Eigen::VectorXd& qdot) const;
        bool getMotorVelocity(Eigen::VectorXd& qdot) const;
        bool getJointAcceleration(Eigen::VectorXd& qddot) const;
        bool getJointEffort(Eigen::VectorXd& tau) const;
        bool getTemperature(Eigen::VectorXd& temp) const;

        bool getJointPosition(std::map<int, double>& q) const;
        bool getMotorPosition(std::map<int, double>& q) const;
        bool getJointVelocity(std::map<int, double>& qdot) const;
        bool getMotorVelocity(std::map<int, double>& qdot) const;
        bool getJointAcceleration(std::map<int, double>& qddot) const;
        bool getJointEffort(std::map<int, double>& tau) const;
        bool getTemperature(std::map<int, double>& temp) const;

        bool getJointPosition(std::map<std::string, double>& q) const;
        bool getMotorPosition(std::map<std::string, double>& q) const;
        bool getJointVelocity(std::map<std::string, double>& qdot) const;
        bool getMotorVelocity(std::map<std::string, double>& qdot) const;
        bool getJointAcceleration(std::map<std::string, double>& qddot) const;
        bool getJointEffort(std::map<std::string, double>& tau) const;
        bool getTemperature(std::map<std::string, double>& temp) const;

        // Setters for RX
        
        bool setJointPosition(const Eigen::VectorXd& q);
        bool setMotorPosition(const Eigen::VectorXd& q);
        bool setJointVelocity(const Eigen::VectorXd& qdot);
        bool setMotorVelocity(const Eigen::VectorXd& qdot);
        bool setJointAcceleration(const Eigen::VectorXd& qddot);
        bool setJointEffort(const Eigen::VectorXd& tau);
        bool setTemperature(const Eigen::VectorXd& temp);
        
        bool setJointPosition(const std::map<int, double>& q);
        bool setMotorPosition(const std::map<int, double>& q);
        bool setJointVelocity(const std::map<int, double>& qdot);
        bool setMotorVelocity(const std::map<int, double>& qdot);
        bool setJointAcceleration(const std::map<int, double>& qddot);
        bool setJointEffort(const std::map<int, double>& tau);
        bool setTemperature(const std::map<int, double>& temp);
        
        bool setJointPosition(const std::map<std::string, double>& q);
        bool setMotorPosition(const std::map<std::string, double>& q);
        bool setJointVelocity(const std::map<std::string, double>& qdot);
        bool setMotorVelocity(const std::map<std::string, double>& qdot);
        bool setJointAcceleration(const std::map<std::string, double>& qddot);
        bool setJointEffort(const std::map<std::string, double>& tau);
        bool setTemperature(const std::map<std::string, double>& temp);
        
        // Getters for TX

        bool getPositionReference(Eigen::VectorXd& q) const;
        bool getVelocityReference(Eigen::VectorXd& qdot) const;
        bool getEffortReference(Eigen::VectorXd& tau) const;
        bool getStiffness(Eigen::VectorXd& K) const;
        bool getDamping(Eigen::VectorXd& D) const;
        
        bool getPositionReference(std::map<int, double>& q) const;
        bool getVelocityReference(std::map<int, double>& qdot) const;
        bool getEffortReference(std::map<int, double>& tau) const;
        bool getStiffness(std::map<int, double>& K) const;
        bool getDamping(std::map<int, double>& D) const;
        
        bool getPositionReference(std::map<std::string, double>& q) const;
        bool getVelocityReference(std::map<std::string, double>& qdot) const;
        bool getEffortReference(std::map<std::string, double>& tau) const;
        bool getStiffness(std::map<std::string, double>& K) const;
        bool getDamping(std::map<std::string, double>& D) const;
        
        // Setters for TX
        
        bool setPositionReference(const Eigen::VectorXd& q);
        bool setVelocityReference(const Eigen::VectorXd& qdot);
        bool setEffortReference(const Eigen::VectorXd& tau);
        bool setStiffness(const Eigen::VectorXd& K);
        bool setDamping(const Eigen::VectorXd& D);
        
        bool setPositionReference(const std::map<int, double>& q);
        bool setVelocityReference(const std::map<int, double>& qdot);
        bool setEffortReference(const std::map<int, double>& tau);
        bool setStiffness(const std::map<int, double>& K);
        bool setDamping(const std::map<int, double>& D);
        
        bool setPositionReference(const std::map<std::string, double>& q);
        bool setVelocityReference(const std::map<std::string, double>& qdot);
        bool setEffortReference(const std::map<std::string, double>& tau);
        bool setStiffness(const std::map<std::string, double>& K);
        bool setDamping(const std::map<std::string, double>& D);



        // Joint limits

        /**
        * @brief Gets a vector of the robot joint limits, as specified in the URDF file. 
        * 
        * @param q_min The output vector of the chain joints lower limits.
        * @param q_max The output vector of the chain joints upper limits.
        * @return void
        */
        void getJointLimits(Eigen::VectorXd& q_min, Eigen::VectorXd& q_max) const;
        
        /**
        * @brief Gets a vector of the robot joint velocity limits, as specified in the URDF file. 
        * 
        * @param qdot_max The output vector of the chain joints velocity limits
        * @return void
        */
        void getVelocityLimits(Eigen::VectorXd& qdot_max) const;
        
        /**
        * @brief Gets a vector of the robot joint effort limits, as specified in the URDF file. 
        * 
        * @param qdot_max The output vector of the chain joints effort limits
        * @return void
        */
        void getEffortLimits(Eigen::VectorXd& tau_max) const;
            
        /**
        * @brief Check the input joint position vector against joint limits. The names of joints violating the
        * limits are pushed into the violating_joints vector (which is not cleared before being filled)
        * 
        * @param q A joint position vector to be checked against joint limits.
        * @param violating_joints The vector of the names of joints violating the limits. Note that this is not cleared before use.
        * @return True if all joints are within their limits.
        */
        bool checkJointLimits(const Eigen::VectorXd& q, 
                              std::vector<std::string>& violating_joints) const;
                              
        /**
        * @brief Check the input joint velocity vector against joint limits. The names of joints violating the
        * limits are pushed into the violating_joints vector (which is not cleared before being filled)
        * 
        * @param qdot A joint position vector to be checked against joint limits.
        * @param violating_joints The vector of the names of joints violating the limits. Note that this is not cleared before use.
        * @return True if all joints are within their limits.
        */
        bool checkVelocityLimits(const Eigen::VectorXd& qdot, 
                                 std::vector<std::string>& violating_joints) const;
                                 
        /**
        * @brief Check the input joint effort vector against joint limits. The names of joints violating the
        * limits are pushed into the violating_joints vector (which is not cleared before being filled)
        * 
        * @param tau A joint position vector to be checked against joint limits.
        * @param violating_joints The vector of the names of joints violating the limits. Note that this is not cleared before use.
        * @return True if all joints are within their limits.
        */
        bool checkEffortLimits(const Eigen::VectorXd& tau, 
                               std::vector<std::string>& violating_joints) const;


        /**
        * @brief Check the input joint position vector against joint limits. 
        * 
        * @param q A joint position vector to be checked against joint limits.
        * @return True if all joints are within their limits.
        */
        bool checkJointLimits(const Eigen::VectorXd& q) const;
        
        /**
        * @brief Check the input joint velocity vector against joint limits. 
        * 
        * @param qdot A joint velocity vector to be checked against joint limits.
        * @return True if all joints are within their limits.
        */
        bool checkVelocityLimits(const Eigen::VectorXd& qdot) const;
        
        /**
        * @brief Check the input joint effort vector against joint limits. 
        * 
        * @param tau A joint effort vector to be checked against joint limits.
        * @return True if all joints are within their limits.
        */
        bool checkEffortLimits(const Eigen::VectorXd& tau) const;
        
        template <typename... SyncFlags>
        bool sync(const XBotInterface& other, SyncFlags... flags);
        
        const std::map<std::string, XBot::KinematicChain::Ptr>&  getChainMap() const;
        
        friend std::ostream& operator<<(std::ostream& os, const XBot::XBotInterface& robot);
        

    protected:
        
        
        
        // Chain getter for developers

        const std::map<std::string, ForceTorqueSensor::Ptr>& getForceTorqueInternal() const; // TBD change the Internal with something more meaningful
        const std::map<std::string, ImuSensor::Ptr>& getImuInternal() const; // TBD change the Internal with something more meaningful
        
        
        
        virtual bool init_internal(const std::string& path_to_cfg){ return true; }
        
        const std::string& getPathToConfig() const;
        
        static bool computeAbsolutePath( const std::string& input_path,
                                         const std::string& midlle_path,
                                         std::string& absolute_path,
                                         std::string extension = "");
        
        const std::vector<std::string>& getModelOrderedChainName() const;

               
        // internal XBotCoreModel object: it does the trick using URDF, SRDF and joint map configuration
        XBotCoreModel _XBotModel;
                
        // TBD avoid protected members in subclasses (using + getters)
        std::map<std::string, XBot::KinematicChain::Ptr> _chain_map;
        std::vector<Joint::Ptr> _ordered_joint_vector;
        std::map<std::string, ForceTorqueSensor::Ptr> _ft_map;
        std::map<std::string, ImuSensor::Ptr> _imu_map;
        std::vector<std::string> _ordered_chain_names;
        
        std::map<int, int> _joint_id_to_eigen_id;
        std::map<std::string, int> _joint_name_to_eigen_id;

    private:

        int _joint_num;
        
        std::string _urdf_string, _srdf_string;
        std::string _urdf_path;
        std::string _srdf_path;
        std::string _joint_map_config;
        
        std::vector<std::string> _ordered_joint_name;
        std::vector<int> _ordered_joint_id;
        
        std::string _path_to_cfg;
        
        bool parseYAML(const std::string &path_to_cfg);


    };
    
    std::ostream& operator<<(std::ostream& os, const XBot::XBotInterface& robot);
    
    template <typename... SyncFlags>
    bool XBot::XBotInterface::sync(const XBot::XBotInterface &other, SyncFlags... flags)
{
    bool success = true;
    for (const auto & c : other._chain_map) {
        const std::string &chain_name = c.first;
        const KinematicChain &chain = *c.second;
        if (_chain_map.count(chain_name)) {
            _chain_map.at(chain_name)->syncFrom(chain, flags...);
            
        } else {
            if(!chain.isVirtual()){
                std::cerr << "ERROR " << __func__ << " : you are trying to synchronize XBotInterfaces with different chains!!" << std::endl;
                success = false;
            }
        }
    }
    return success;
}

}

#endif // __I_XBOT_INTERFACE_H__