#include <XBotInterface/IXBotInterface.h>


XBot::IXBotInterface::IXBotInterface()
{

}

XBot::IXBotInterface::IXBotInterface(const XBot::IXBotInterface &other):
    _joint_num(other._joint_num),
    _ordered_joint_name(other._ordered_joint_name),
    _ordered_joint_id(other._ordered_joint_id),
    _XBotModel(other._XBotModel),
    _urdf_string(other._urdf_string),
    _srdf_string(other._srdf_string)
{

    for (const auto & chain_name_ptr_pair : other._chain_map) {

        const std::string &chain_name = chain_name_ptr_pair.first;
        const XBot::KinematicChain::Ptr &other_chainptr = chain_name_ptr_pair.second;

        XBot::KinematicChain::Ptr chainptr = std::make_shared<XBot::KinematicChain>();
        *chainptr = *other_chainptr;

        _chain_map[chain_name] = chainptr;

    }

}

bool XBot::IXBotInterface::init(const std::string &path_to_cfg)
{
    std::ifstream fin(path_to_cfg);
    if (fin.fail()) {
        printf("Can not open %s\n", path_to_cfg.c_str());    //TBD change it
        return false;
    }

    YAML::Node root_cfg = YAML::LoadFile(path_to_cfg);
    const YAML::Node &x_bot_interface = root_cfg["x_bot_interface"]; // TBD check if exists
    std::string _urdf_path = x_bot_interface["urdf_path"].as<std::string>();
    std::string _srdf_path = x_bot_interface["srdf_path"].as<std::string>();
    std::string _joint_map_config = x_bot_interface["joint_map_config"].as<std::string>();

    // initialize the model
    if (!_XBotModel.init(_urdf_path, _srdf_path, _joint_map_config)) {
        printf("ERROR: model initialization failed, please check the urdf_path and srdf_path in your YAML config file.\n"); //TBD change it
        return false;
    }
    // generate the robot
    _XBotModel.generate_robot();

    // construct class
    _joint_num = _XBotModel.get_joint_num();
    _urdf_string = _XBotModel.get_urdf_string();
    _srdf_string = _XBotModel.get_srdf_string();
    _XBotModel.get_enabled_joint_ids(_ordered_joint_id);
    _XBotModel.get_enabled_joint_names(_ordered_joint_name);

    for (const std::string & chain_name : _XBotModel.get_chain_names()) {
        XBot::KinematicChain::Ptr actual_chain = std::make_shared<KinematicChain>(chain_name,
                _XBotModel);
        _chain_map[chain_name] = actual_chain;
    }

    // call virtual init_internal
    return init_internal(path_to_cfg);
}


XBot::KinematicChain &XBot::IXBotInterface::operator()(const std::string &chain_name)
{
    if (_chain_map.count(chain_name)) {
        return *_chain_map.at(chain_name);
    }
    std::cerr << "ERROR " << __func__ << " : you are requesting a chain with name " << chain_name << " that does not exists!!" << std::endl;
    return _dummy_chain;
}


XBot::KinematicChain &XBot::IXBotInterface::leg(int id)
{
    if (_XBotModel.get_legs_chain().size() > id) {
        const std::string &requested_leg_name = _XBotModel.get_legs_chain().at(id);
        return *_chain_map.at(requested_leg_name);
    }
    std::cerr << "ERROR " << __func__ << " : you are requesting a legs with id " << id << " that does not exists!!" << std::endl;
    return _dummy_chain;
}

int XBot::IXBotInterface::legs() const
{
    return _XBotModel.get_legs_chain().size();
}

XBot::KinematicChain &XBot::IXBotInterface::arm(int id)
{
    if (_XBotModel.get_arms_chain().size() > id) {
        const std::string &requested_arm_name = _XBotModel.get_arms_chain().at(id);
        return *_chain_map.at(requested_arm_name);
    }
    std::cerr << "ERROR " << __func__ << " : you are requesting a arms with id " << id << " that does not exists!!" << std::endl;
    return _dummy_chain;
}

int XBot::IXBotInterface::arms() const
{
    return _XBotModel.get_arms_chain().size();
}

bool XBot::IXBotInterface::sync(const XBot::IXBotInterface &other)
{
    for (const auto & c : other._chain_map) {
        const std::string &chain_name = c.first;
        const KinematicChain &chain = *c.second;
        if (_chain_map.count(chain_name)) {
            _chain_map.at(chain_name)->sync(chain);
        } else {
            std::cerr << "ERROR " << __func__ << " : you are trying to synchronize IXBotInterfaces with different chains!!" << std::endl;
        }
    }
}


bool XBot::IXBotInterface::hasJoint(const std::string &joint_name) const
{
    return std::find(_ordered_joint_name.begin(), _ordered_joint_name.end(), joint_name) !=  _ordered_joint_name.end();
}





bool XBot::IXBotInterface::getLinkPos(Eigen::VectorXd &q) const
{
    if (q.rows() != _joint_num) {
        q.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            q[q_index++] = _chain_map.at(chain_name)->getLinkPos(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getLinkPos(std::map< std::string, double > &q) const
{
    q.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            q[chain.jointName(j)] = chain.getLinkPos(j);
        }

    }
}

bool XBot::IXBotInterface::getLinkPos(std::map< int, double > &q) const
{
    q.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            q[chain.jointId(j)] = chain.getLinkPos(j);
        }

    }
}

bool XBot::IXBotInterface::getPosRef(std::map< std::string, double > &q) const
{
    q.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            q[chain.jointName(j)] = chain.getPosRef(j);
        }

    }
}

bool XBot::IXBotInterface::getPosRef(std::map< int, double > &q) const
{
    q.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            q[chain.jointId(j)] = chain.getPosRef(j);
        }

    }
}

bool XBot::IXBotInterface::getPosRef(Eigen::VectorXd &q) const
{
    if (q.rows() != _joint_num) {
        q.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            q[q_index++] = _chain_map.at(chain_name)->getPosRef(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getDamping(std::map< std::string, double > &D) const
{
    D.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            D[chain.jointName(j)] = chain.getDamping(j);
        }

    }
}

bool XBot::IXBotInterface::getEffort(std::map< std::string, double > &tau) const
{
    tau.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            tau[chain.jointName(j)] = chain.getEffort(j);
        }

    }
}

bool XBot::IXBotInterface::getEffortRef(std::map< std::string, double > &tau) const
{
    tau.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            tau[chain.jointName(j)] = chain.getEffortRef(j);
        }

    }
}

bool XBot::IXBotInterface::getLinkVel(std::map< std::string, double > &qdot) const
{
    qdot.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            qdot[chain.jointName(j)] = chain.getLinkVel(j);
        }

    }
}

bool XBot::IXBotInterface::getMotorPos(std::map< std::string, double > &q) const
{
    q.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            q[chain.jointName(j)] = chain.getMotorPos(j);
        }

    }
}

bool XBot::IXBotInterface::getStiffness(std::map< std::string, double > &K) const
{
    K.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            K[chain.jointName(j)] = chain.getStiffness(j);
        }

    }
}

bool XBot::IXBotInterface::getTemperature(std::map< std::string, double > &temp) const
{
    temp.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            temp[chain.jointName(j)] = chain.getTemperature(j);
        }

    }
}

bool XBot::IXBotInterface::getVelRef(std::map< std::string, double > &qdot) const
{
    qdot.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            qdot[chain.jointName(j)] = chain.getVelRef(j);
        }

    }
}

bool XBot::IXBotInterface::getMotorVel(std::map< std::string, double > &qdot) const
{
    qdot.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            qdot[chain.jointName(j)] = chain.getMotorVel(j);
        }

    }
}



bool XBot::IXBotInterface::getDamping(std::map< int, double > &D) const
{
    D.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            D[chain.jointId(j)] = chain.getDamping(j);
        }

    }
}

bool XBot::IXBotInterface::getEffort(std::map< int, double > &tau) const
{
    tau.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            tau[chain.jointId(j)] = chain.getEffort(j);
        }

    }
}

bool XBot::IXBotInterface::getEffortRef(std::map< int, double > &tau) const
{
    tau.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            tau[chain.jointId(j)] = chain.getEffortRef(j);
        }

    }
}


bool XBot::IXBotInterface::getVelRef(std::map< int, double > &qdot) const
{
    qdot.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            qdot[chain.jointId(j)] = chain.getVelRef(j);
        }

    }
}


bool XBot::IXBotInterface::getLinkVel(std::map< int, double > &qdot) const
{
    qdot.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            qdot[chain.jointId(j)] = chain.getLinkVel(j);
        }

    }
}

bool XBot::IXBotInterface::getMotorPos(std::map< int, double > &q) const
{
    q.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            q[chain.jointId(j)] = chain.getMotorPos(j);
        }

    }
}

bool XBot::IXBotInterface::getMotorVel(std::map< int, double > &qdot) const
{
    qdot.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            qdot[chain.jointId(j)] = chain.getMotorVel(j);
        }

    }
}

bool XBot::IXBotInterface::getStiffness(std::map< int, double > &K) const
{
    K.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            K[chain.jointId(j)] = chain.getStiffness(j);
        }

    }
}

bool XBot::IXBotInterface::getTemperature(std::map< int, double > &temp) const
{
    temp.clear();

    for (const auto & chain_name_ptr_pair : _chain_map) {

        const KinematicChain &chain = *chain_name_ptr_pair.second;

        for (int j = 0; j < chain.getJointNum(); j++) {
            temp[chain.jointId(j)] = chain.getTemperature(j);
        }

    }
}




bool XBot::IXBotInterface::getDamping(Eigen::VectorXd &D) const
{
    if (D.rows() != _joint_num) {
        D.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            D[q_index++] = _chain_map.at(chain_name)->getDamping(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getEffort(Eigen::VectorXd &tau) const
{
    if (tau.rows() != _joint_num) {
        tau.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            tau[q_index++] = _chain_map.at(chain_name)->getEffort(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getEffortRef(Eigen::VectorXd &tau) const
{
    if (tau.rows() != _joint_num) {
        tau.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            tau[q_index++] = _chain_map.at(chain_name)->getEffortRef(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getLinkVel(Eigen::VectorXd &qdot) const
{
    if (qdot.rows() != _joint_num) {
        qdot.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            qdot[q_index++] = _chain_map.at(chain_name)->getLinkVel(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getMotorPos(Eigen::VectorXd &q) const
{
    if (q.rows() != _joint_num) {
        q.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            q[q_index++] = _chain_map.at(chain_name)->getMotorPos(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getMotorVel(Eigen::VectorXd &qdot) const
{
    if (qdot.rows() != _joint_num) {
        qdot.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            qdot[q_index++] = _chain_map.at(chain_name)->getMotorVel(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getStiffness(Eigen::VectorXd &K) const
{
    if (K.rows() != _joint_num) {
        K.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            K[q_index++] = _chain_map.at(chain_name)->getStiffness(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getTemperature(Eigen::VectorXd &temp) const
{
    if (temp.rows() != _joint_num) {
        temp.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            temp[q_index++] = _chain_map.at(chain_name)->getTemperature(i);
        }
    }
    return true;
}

bool XBot::IXBotInterface::getVelRef(Eigen::VectorXd &qdot) const
{
    if (qdot.rows() != _joint_num) {
        qdot.resize(_joint_num);
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            qdot[q_index++] = _chain_map.at(chain_name)->getVelRef(i);
        }
    }
    return true;
}


bool XBot::IXBotInterface::setLinkPos(const std::map< std::string, double > &q)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;
        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (q.count(joint_name)) {
                chain.setLinkPos(i, q.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }
    }
    return success;
}

bool XBot::IXBotInterface::setMotorPos(const std::map< std::string, double > &q)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (q.count(joint_name)) {
                chain.setMotorPos(i, q.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setPosRef(const std::map< std::string, double > &q)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (q.count(joint_name)) {
                chain.setPosRef(i, q.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setEffort(const std::map< std::string, double > &tau)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (tau.count(joint_name)) {
                chain.setEffort(i, tau.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setEffortRef(const std::map< std::string, double > &tau)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (tau.count(joint_name)) {
                chain.setEffortRef(i, tau.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setLinkVel(const std::map< std::string, double > &qdot)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (qdot.count(joint_name)) {
                chain.setLinkVel(i, qdot.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setMotorVel(const std::map< std::string, double > &qdot)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (qdot.count(joint_name)) {
                chain.setMotorVel(i, qdot.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setVelRef(const std::map< std::string, double > &qdot)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (qdot.count(joint_name)) {
                chain.setVelRef(i, qdot.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setTemperature(const std::map< std::string, double > &temp)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (temp.count(joint_name)) {
                chain.setTemperature(i, temp.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setStiffness(const std::map< std::string, double > &K)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (K.count(joint_name)) {
                chain.setStiffness(i, K.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setDamping ( const Eigen::VectorXd& D )
{
     if (D.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : D has wrong size " << D.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setDamping(i, D[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setEffort ( const Eigen::VectorXd& tau )
{
     if (tau.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : tau has wrong size " << tau.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setEffort(i, tau[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setEffortRef ( const Eigen::VectorXd& tau )
{
     if (tau.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : tau has wrong size " << tau.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setEffort(i, tau[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setLinkPos ( const Eigen::VectorXd& q )
{
     if (q.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : q has wrong size " << q.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setLinkPos(i, q[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setLinkVel ( const Eigen::VectorXd& qdot )
{
     if (qdot.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : qdot has wrong size " << qdot.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setLinkVel(i, qdot[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setMotorPos ( const Eigen::VectorXd& q )
{
     if (q.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : q has wrong size " << q.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setMotorPos(i, q[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setMotorVel ( const Eigen::VectorXd& qdot )
{
     if (qdot.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : qdot has wrong size " << qdot.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setMotorVel(i, qdot[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setPosRef ( const Eigen::VectorXd& q )
{
     if (q.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : q has wrong size " << q.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setPosRef(i, q[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setStiffness ( const Eigen::VectorXd& K )
{
     if (K.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : K has wrong size " << K.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setStiffness(i, K[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setTemperature ( const Eigen::VectorXd& temp )
{
     if (temp.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : temp has wrong size " << temp.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setTemperature(i, temp[q_index++]);
        }
    }
    return true;
}

bool XBot::IXBotInterface::setVelRef ( const Eigen::VectorXd& qdot )
{
     if (qdot.rows() != _joint_num) {
        std::cerr << "ERROR in " << __func__ << " : qdot has wrong size " << qdot.rows() << " != robot joint number " << _joint_num << std::endl;
        return false;
    }
    int q_index = 0;
    for (const std::string & chain_name : _XBotModel.get_ordered_chain_names()) {
        for (int i = 0; i < _chain_map.at(chain_name)->getJointNum(); i++) {
            _chain_map.at(chain_name)->setVelRef(i, qdot[q_index++]);
        }
    }
    return true;
}





bool XBot::IXBotInterface::setDamping(const std::map< std::string, double > &D)
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            const std::string &joint_name = chain.jointName(i);
            if (D.count(joint_name)) {
                chain.setDamping(i, D.at(joint_name));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_name << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setDamping ( const std::map< int, double >& D )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (D.count(joint_id)) {
                chain.setDamping(i, D.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}


bool XBot::IXBotInterface::setEffort ( const std::map< int, double >& tau )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (tau.count(joint_id)) {
                chain.setEffort(i, tau.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setEffortRef ( const std::map< int, double >& tau )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (tau.count(joint_id)) {
                chain.setEffortRef(i, tau.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setLinkPos ( const std::map< int, double >& q )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (q.count(joint_id)) {
                chain.setLinkPos(i, q.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setLinkVel ( const std::map< int, double >& qdot )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (qdot.count(joint_id)) {
                chain.setLinkVel(i, qdot.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setMotorPos ( const std::map< int, double >& q )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (q.count(joint_id)) {
                chain.setMotorPos(i, q.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setMotorVel ( const std::map< int, double >& qdot )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (qdot.count(joint_id)) {
                chain.setMotorVel(i, qdot.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setPosRef ( const std::map< int, double >& q )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (q.count(joint_id)) {
                chain.setPosRef(i, q.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setStiffness ( const std::map< int, double >& K )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (K.count(joint_id)) {
                chain.setStiffness(i, K.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setTemperature ( const std::map< int, double >& temp )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (temp.count(joint_id)) {
                chain.setTemperature(i, temp.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}

bool XBot::IXBotInterface::setVelRef ( const std::map< int, double >& qdot )
{
    bool success = true;
    for (const auto & chainname_ptr_pair : _chain_map) {

        KinematicChain &chain = *chainname_ptr_pair.second;

        for (int i = 0; i < chain.getJointNum(); i++) {
            int joint_id = chain.jointId(i);
            if (qdot.count(joint_id)) {
                chain.setVelRef(i, qdot.at(joint_id));
            } else {
                std::cerr << "ERROR in " << __func__ << "! Joint " << joint_id << " is not defined!" << std::endl;
                success = false;
            }
        }

    }
    return success;
}



std::vector< std::string > XBot::IXBotInterface::getChainNames() const
{
    return _XBotModel.get_chain_names();
}

std::map< std::string, XBot::KinematicChain::Ptr > XBot::IXBotInterface::getChainMap() const
{
    return _chain_map;
}


const srdf::Model& XBot::IXBotInterface::getSrdf() const
{
    return _XBotModel;
}

const std::string& XBot::IXBotInterface::getSrdfString() const
{
    return _srdf_string;
}

const urdf::ModelInterface& XBot::IXBotInterface::getUrdf() const
{
    return *(_XBotModel.get_urdf_model());
}

const std::string& XBot::IXBotInterface::getUrdfString() const
{
    return _urdf_string;
}



XBot::IXBotInterface::~IXBotInterface()
{
}

XBot::IXBotInterface &XBot::IXBotInterface::operator=(const XBot::IXBotInterface &rhs)
{
    IXBotInterface tmp(rhs);
    std::swap(_joint_num, tmp._joint_num);
    std::swap(_XBotModel, tmp._XBotModel);
    std::swap(_urdf_string, tmp._urdf_string);
    std::swap(_srdf_string, tmp._srdf_string);
    std::swap(_ordered_joint_name, tmp._ordered_joint_name);
    std::swap(_ordered_joint_id, tmp._ordered_joint_id);
    std::swap(_chain_map, tmp._chain_map);
    std::swap(_dummy_chain, tmp._dummy_chain);

}


const std::vector< std::string > &XBot::IXBotInterface::getEnabledJointNames() const
{
    return _ordered_joint_name;
}

std::ostream& XBot::operator<< ( std::ostream& os, const XBot::IXBotInterface& robot )
{
    os << "Robot name: " << std::endl;
    for( const auto& c : robot.getChainMap() ) {
        os << (*c.second) << std::endl;
    }
    return os;
}


