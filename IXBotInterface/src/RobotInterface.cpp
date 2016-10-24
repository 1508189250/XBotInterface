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

#include <XBotInterface/RobotInterface.h>
#include <ros/ros.h> //TBD: remove include

// NOTE Static members need to be defined in the cpp 
std::string XBot::RobotInterface::_framework;
std::string XBot::RobotInterface::_subclass_name;
std::string XBot::RobotInterface::_path_to_shared_lib;
std::string XBot::RobotInterface::_subclass_factory_name;
XBot::RobotInterface::Ptr XBot::RobotInterface::_instance_ptr;
XBot::ModelInterface::Ptr XBot::RobotInterface::_model;
shlibpp::SharedLibraryClass<XBot::RobotInterface> XBot::RobotInterface::_robot_interface_instance;
shlibpp::SharedLibraryClassFactory<XBot::RobotInterface> XBot::RobotInterface::_robot_interface_factory;

XBot::RobotInterface::RobotInterface()
{
}

bool XBot::RobotInterface::parseYAML(const std::string &path_to_cfg)
{
    std::ifstream fin(path_to_cfg);
    if (fin.fail()) {
        printf("Can not open %s\n", path_to_cfg.c_str());
        return false;
    }

    // loading YAML
    YAML::Node root_cfg = YAML::LoadFile(path_to_cfg);
    YAML::Node x_bot_interface;
    // XBotInterface info
    if(root_cfg["x_bot_interface"]) {
        x_bot_interface = root_cfg["x_bot_interface"]; 
    }
    else {
        std::cerr << "ERROR in " << __func__ << " : YAML file  " << path_to_cfg << "  does not contain x_bot_interface mandatory node!!" << std::endl;
        return false;
    }
    // check framework
    if(x_bot_interface["framework"]) {
        _framework = x_bot_interface["framework"].as<std::string>();
    }
    else {
        std::cerr << "ERROR in " << __func__ << " : x_bot_interface node of  " << path_to_cfg << "  does not contain framework mandatory node!!" << std::endl;
        return false;
    }
    
    // subclass forced
    _subclass_name = std::string("RobotInterface") + _framework;
    // check the path to shared lib
    if(root_cfg[_subclass_name]["path_to_shared_lib"]) {
        computeAbsolutePath(root_cfg[_subclass_name]["path_to_shared_lib"].as<std::string>(), 
                            LIB_MIDDLE_PATH,
                            _path_to_shared_lib); 
    }
    else {
        std::cerr << "ERROR in " << __func__ << " : x_bot_interface node of  " << path_to_cfg << "  does not contain path_to_shared_lib mandatory node!!" << std::endl;
        return false;
    }
    
    if(root_cfg[_subclass_name]["subclass_factory_name"]) {
        _subclass_factory_name = root_cfg[_subclass_name]["subclass_factory_name"].as<std::string>();
    }
    else {
        std::cerr << "ERROR in " << __func__ << " : x_bot_interface node of  " << path_to_cfg << "  does not contain subclass_factory_name mandatory node!!" << std::endl;
        return false;
    }
    return true;

}


XBot::RobotInterface::Ptr XBot::RobotInterface::getRobot(const std::string &path_to_cfg, int argc, char **argv)
{
    // NOTE singleton
    if (_instance_ptr) {
        return _instance_ptr;
    }
    // parsing YAML
    if (!parseYAML(path_to_cfg)) {
        std::cerr << "ERROR in " << __func__ << " : could not parse the YAML " << path_to_cfg << " . See error above!!" << std::endl;
        return _instance_ptr;
    }
    
    if (_framework == "ROS") {
        ros::init(argc, argv, "from_config"); // TBD remove this ugly reference to implementation details
    }

    // loading the requested robot interface
    _robot_interface_factory.open( _path_to_shared_lib.c_str(),
                                                                                _subclass_factory_name.c_str());
    if (!_robot_interface_factory.isValid()) {
        // NOTE print to celebrate the wizard
        printf("error (%s) : %s\n", shlibpp::Vocab::decode(_robot_interface_factory.getStatus()).c_str(),
               _robot_interface_factory.getLastNativeError().c_str());
    }
    // open and init robot interface
    _robot_interface_instance.open(_robot_interface_factory); 
    _robot_interface_instance->init(path_to_cfg);
    // static instance of the robot interface
    _instance_ptr = std::shared_ptr<RobotInterface>(&_robot_interface_instance.getContent(), [](RobotInterface* ptr){return;});
    
    // loading the requested model interface internal to the robot
    _model = XBot::ModelInterface::getModel(path_to_cfg);
    
    return _instance_ptr;

}

XBot::ModelInterface& XBot::RobotInterface::model()
{
    return *_model;
}


const std::vector< std::string >& XBot::RobotInterface::getModelOrderedChainName()
{

    return IXBotInterface::getModelOrderedChainName();
}



bool XBot::RobotInterface::sense(bool sync_model)
{
    bool sense_ok = sense_internal();
    if (sync_model) {
        return sense_ok && (_model->syncFrom(*this));
    }
    return sense_ok;
}


bool XBot::RobotInterface::move(bool sync_model)
{
    bool sync_ok = true;
    if (sync_model) {
        sync_ok = this->setReferenceFrom(*_model);
    }
    return sync_ok && move_internal();
}

bool XBot::RobotInterface::init_internal(const std::string& path_to_cfg)
{
    // Fill _robot_chain_map with shallow copies of chains in _chain_map
    
    _robot_chain_map.clear();
    
    for( const auto& c : _chain_map){
        
        RobotChain::Ptr robot_chain(new RobotChain());
        robot_chain->shallowCopy(*c.second);
        
        _robot_chain_map[c.first] = robot_chain;
        
    }
    
    // Call vitual init_robot
    
    return init_robot(path_to_cfg);
}

XBot::RobotChain& XBot::RobotInterface::arm(int arm_id)
{
    if (_XBotModel.get_arms_chain().size() > arm_id) {
        const std::string &requested_arm_name = _XBotModel.get_arms_chain().at(arm_id);
        return *_robot_chain_map.at(requested_arm_name);
    }
    std::cerr << "ERROR " << __func__ << " : you are requesting a arms with id " << arm_id << " that does not exists!!" << std::endl;
    return _dummy_chain;
}

XBot::RobotChain& XBot::RobotInterface::leg(int leg_id)
{
    if (_XBotModel.get_legs_chain().size() > leg_id) {
        const std::string &requested_leg_name = _XBotModel.get_legs_chain().at(leg_id);
        return *_robot_chain_map.at(requested_leg_name);
    }
    std::cerr << "ERROR " << __func__ << " : you are requesting a legs with id " << leg_id << " that does not exists!!" << std::endl;
    return _dummy_chain;
}

XBot::RobotChain& XBot::RobotInterface::operator()(const std::string& chain_name)
{
    if (_robot_chain_map.count(chain_name)) {
        return *_robot_chain_map.at(chain_name);
    }
    std::cerr << "ERROR " << __func__ << " : you are requesting a chain with name " << chain_name << " that does not exists!!" << std::endl;
    return _dummy_chain;
}

bool XBot::RobotInterface::setReferenceFrom ( const XBot::ModelInterface& model )
{
    bool success = true;
    for (const auto & c : model._model_chain_map) {
        const std::string &chain_name = c.first;
        const ModelChain &chain = *c.second;
        if (_robot_chain_map.count(chain_name)) {
            _robot_chain_map.at(chain_name)->setReferenceFrom(chain);
        } else {
            if(!chain.isVirtual()){
                std::cerr << "ERROR " << __func__ << " : you are trying to synchronize IXBotInterfaces with different chains!!" << std::endl;
                success = false;
            }
        }
    }
    return success;
}

bool XBot::RobotInterface::getDamping ( std::map< std::string, double >& D ) const
{
     return XBot::IXBotInterface::getDamping ( D );
}

bool XBot::RobotInterface::getDamping ( std::map< int, double >& D ) const
{
     return XBot::IXBotInterface::getDamping ( D );
}

bool XBot::RobotInterface::getDamping ( Eigen::VectorXd& D ) const
{
     return XBot::IXBotInterface::getDamping ( D );
}

bool XBot::RobotInterface::getEffortReference ( std::map< std::string, double >& tau ) const
{
     return XBot::IXBotInterface::getEffortReference ( tau );
}

bool XBot::RobotInterface::getEffortReference ( std::map< int, double >& tau ) const
{
     return XBot::IXBotInterface::getEffortReference ( tau );
}

bool XBot::RobotInterface::getEffortReference ( Eigen::VectorXd& tau ) const
{
     return XBot::IXBotInterface::getEffortReference ( tau );
}

bool XBot::RobotInterface::getPositionReference ( std::map< std::string, double >& q ) const
{
     return XBot::IXBotInterface::getPositionReference ( q );
}

bool XBot::RobotInterface::getPositionReference ( std::map< int, double >& q ) const
{
     return XBot::IXBotInterface::getPositionReference ( q );
}

bool XBot::RobotInterface::getPositionReference ( Eigen::VectorXd& q ) const
{
     return XBot::IXBotInterface::getPositionReference ( q );
}

bool XBot::RobotInterface::getStiffness ( std::map< std::string, double >& K ) const
{
     return XBot::IXBotInterface::getStiffness ( K );
}

bool XBot::RobotInterface::getStiffness ( std::map< int, double >& K ) const
{
     return XBot::IXBotInterface::getStiffness ( K );
}

bool XBot::RobotInterface::getStiffness ( Eigen::VectorXd& K ) const
{
     return XBot::IXBotInterface::getStiffness ( K );
}

bool XBot::RobotInterface::getVelocityReference ( std::map< std::string, double >& qdot ) const
{
     return XBot::IXBotInterface::getVelocityReference ( qdot );
}

bool XBot::RobotInterface::getVelocityReference ( std::map< int, double >& qdot ) const
{
     return XBot::IXBotInterface::getVelocityReference ( qdot );
}

bool XBot::RobotInterface::getVelocityReference ( Eigen::VectorXd& qdot ) const
{
     return XBot::IXBotInterface::getVelocityReference ( qdot );
}

bool XBot::RobotInterface::setDamping ( const std::map< std::string, double >& D )
{
     return XBot::IXBotInterface::setDamping ( D );
}

bool XBot::RobotInterface::setDamping ( const std::map< int, double >& D )
{
     return XBot::IXBotInterface::setDamping ( D );
}

bool XBot::RobotInterface::setDamping ( const Eigen::VectorXd& D )
{
     return XBot::IXBotInterface::setDamping ( D );
}

bool XBot::RobotInterface::setEffortReference ( const std::map< std::string, double >& tau )
{
     return XBot::IXBotInterface::setEffortReference ( tau );
}

bool XBot::RobotInterface::setEffortReference ( const std::map< int, double >& tau )
{
     return XBot::IXBotInterface::setEffortReference ( tau );
}

bool XBot::RobotInterface::setEffortReference ( const Eigen::VectorXd& tau )
{
     return XBot::IXBotInterface::setEffortReference ( tau );
}

bool XBot::RobotInterface::setPositionReference ( const std::map< std::string, double >& q )
{
     return XBot::IXBotInterface::setPositionReference ( q );
}

bool XBot::RobotInterface::setPositionReference ( const std::map< int, double >& q )
{
     return XBot::IXBotInterface::setPositionReference ( q );
}

bool XBot::RobotInterface::setPositionReference ( const Eigen::VectorXd& q )
{
     return XBot::IXBotInterface::setPositionReference ( q );
}

bool XBot::RobotInterface::setStiffness ( const std::map< std::string, double >& K )
{
     return XBot::IXBotInterface::setStiffness ( K );
}

bool XBot::RobotInterface::setStiffness ( const std::map< int, double >& K )
{
     return XBot::IXBotInterface::setStiffness ( K );
}

bool XBot::RobotInterface::setStiffness ( const Eigen::VectorXd& K )
{
     return XBot::IXBotInterface::setStiffness ( K );
}

bool XBot::RobotInterface::setVelocityReference ( const std::map< std::string, double >& qdot )
{
     return XBot::IXBotInterface::setVelocityReference ( qdot );
}

bool XBot::RobotInterface::setVelocityReference ( const std::map< int, double >& qdot )
{
     return XBot::IXBotInterface::setVelocityReference ( qdot );
}

bool XBot::RobotInterface::setVelocityReference ( const Eigen::VectorXd& qdot )
{
     return XBot::IXBotInterface::setVelocityReference ( qdot );
}


