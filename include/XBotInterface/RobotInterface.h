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

#ifndef __ROBOT_INTERFACE_H__
#define __ROBOT_INTERFACE_H__

#include <vector>
#include <map>
#include <memory>
#include <SharedLibraryClass.h>

#include <cstdlib>

#include <XBotInterface/XBotInterface.h>
#include <XBotInterface/ModelInterface.h>
#include <XBotInterface/RobotChain.h>

namespace XBot
{

class RobotInterface : public XBotInterface
{

public:
    
    struct Options : public XBotInterface::Options {
        
        std::string framework_name;
        std::string robot_subclass_factory_name;
        
        ModelInterface::Options internal_model_options;
        
    };

    RobotInterface();

    typedef std::shared_ptr<RobotInterface> Ptr;

    static RobotInterface::Ptr getRobot(const std::string &path_to_cfg, int argc, char **argv);
    
    ModelInterface& model();
    RobotInterface& operator=(const RobotInterface& other) = delete;
    RobotInterface(const RobotInterface& other) = delete;
    
    virtual double getTime() const = 0;
    virtual bool isRunning() const = 0;

    bool sense(bool sync_model = true);
    bool move();
    
    RobotChain& operator()(const std::string& chain_name);
    RobotChain& chain(const std::string& chain_name);
    RobotChain& arm(int arm_id);
    RobotChain& leg(int leg_id);
    
    /**
     * @brief Sets the robot references according to a ModelInterface.
     * Flags can be specified to select a part of the state to be synchronized.
     * 
     * @usage robot.setReferenceFrom(model, XBot::Sync::Position, XBot::Sync::Effort)
     * @usage robot.setReferenceFrom(other_model, XBot::Sync::Position)
     * 
     * @param model The ModelInterface whose state is used as a reference for the robot.
     * model.
     * @param flags Flags to specify what part of the model state must be used a reference. By default (i.e. if
     * this argument is omitted) the whole state is used. Otherwise, an arbitrary number of flags
     * can be specified in order to select a subset of the state. The flags must be of the enum type
     * XBot::Sync, which can take the following values:
     *  - Sync::Position, 
     *  - Sync::Velocity
     *  - Sync::Acceleration
     *  - Sync::Effort
     *  - Sync::Stiffness 
     *  - Sync::Damping 
     *  - Sync::Impedance
     *  - Sync::All

     * @return True if the synchronization is allowed, false otherwise.
     */
    template <typename... SyncFlags>
    bool setReferenceFrom(const ModelInterface& model, SyncFlags... flags);

    virtual bool setControlMode(const std::map<std::string, std::string> &joint_control_mode_map) = 0;
    virtual bool setControlMode(const std::string &robot_control_mode) = 0;
    virtual bool getControlMode(std::map<std::string, std::string> &joint_control_mode_map) = 0;
    
    // Getters for RX

    using XBotInterface::getJointPosition;
    using XBotInterface::getMotorPosition;
    using XBotInterface::getJointVelocity;
    using XBotInterface::getMotorVelocity;
    using XBotInterface::getJointEffort;
    using XBotInterface::getTemperature;

    
    // Getters for TX

    using XBotInterface::getPositionReference;
    using XBotInterface::getVelocityReference;
    using XBotInterface::getEffortReference;
    using XBotInterface::getStiffness;
    using XBotInterface::getDamping;

    // Setters for TX
    
    using XBotInterface::setPositionReference;
    using XBotInterface::setVelocityReference;
    using XBotInterface::setEffortReference;
    using XBotInterface::setStiffness;
    using XBotInterface::setDamping;
    


protected:

    virtual bool sense_internal() = 0;
    virtual bool move_internal() = 0;
    virtual bool read_sensors() = 0;
    virtual bool init_robot(const std::string& path_to_cfg) = 0;
    
    

    // Setters for RX
    
    using XBotInterface::setJointPosition;
    using XBotInterface::setMotorPosition;
    using XBotInterface::setJointVelocity;
    using XBotInterface::setMotorVelocity;
    using XBotInterface::setJointEffort;
    using XBotInterface::setTemperature;
    
    using XBotInterface::sync;

        


private:
    
    virtual bool init_internal(const std::string &path_to_cfg);
    
    using XBotInterface::_chain_map;
    using XBotInterface::_ordered_joint_vector;
    using XBotInterface::_ordered_chain_names;
    std::map<std::string, XBot::RobotChain::Ptr> _robot_chain_map;
    XBot::RobotChain _dummy_chain;
    
    static RobotInterface::Ptr _instance_ptr;
    static shlibpp::SharedLibraryClass<RobotInterface> _robot_interface_instance;
    static shlibpp::SharedLibraryClassFactory<RobotInterface> _robot_interface_factory;
    static ModelInterface::Ptr _model;
    
    static std::string _framework;
    static std::string _subclass_name;
    static std::string _path_to_shared_lib;
    static std::string _subclass_factory_name;
            
    std::vector<std::string> _model_ordered_chain_name;
    
    static bool parseYAML(const std::string &path_to_cfg);
    


};

template <typename... SyncFlags>
bool XBot::RobotInterface::setReferenceFrom ( const XBot::ModelInterface& model, SyncFlags... flags )
{
    bool success = true;
    for (const auto & c : model._model_chain_map) {
        
        const std::string &chain_name = c.first;
        const ModelChain &chain = *c.second;
        
        if (_robot_chain_map.count(chain_name)) {
            _robot_chain_map.at(chain_name)->setReferenceFrom(chain, flags...);
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

#endif // __ROBOT_INTERFACE_H__