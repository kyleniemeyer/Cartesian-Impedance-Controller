
#include <Eigen/Dense>
#include <iostream>
#include <vector>

#pragma once
class CartesianImpedanceController_base
{

public:
    // Initialization
    bool initialize();

    // Set the desired diagonal stiffnessess + nullspace stiffness
    void set_stiffness(double t_x, double t_y, double t_z, double r_x, double r_y, double r_z, double n);

    // Set the desired damping factors + (TODO) nullspace damping
    void set_damping(double d_x, double d_y, double d_z, double d_a, double d_b, double d_c, double d_n);

    // Set the desired enf-effector pose
    void set_desired_pose(Eigen::Vector3d position_d_, Eigen::Quaterniond orientation_d_);

    // Set the desired nullspace configuration
    void set_nullspace_config(Eigen::Matrix<double, 7, 1> q_d_nullspace_target_);

    // Apply filtering on stiffness + end-effector pose. Default inactive && depends on update_frequency
    void set_filtering(double update_frequency, double filter_params_stiffness, double filter_params_pose, double filter_params_wrench);

    // Returns the desired control law
    Eigen::VectorXd get_commanded_torques(Eigen::Matrix<double, 7, 1> q, Eigen::Matrix<double, 7, 1> dq, Eigen::Vector3d position, Eigen::Quaterniond orientation, Eigen::Matrix<double, 6, 7> jacobian);

    // Get the state of the robot. Updates when "get_commanded_torques" is called
    void get_robot_state(Eigen::Matrix<double, 7, 1> &q, Eigen::Matrix<double, 7, 1> &dq, Eigen::Vector3d &position, Eigen::Quaterniond &orientation, Eigen::Vector3d &position_d_, Eigen::Quaterniond &orientation_d_, Eigen::Matrix<double, 6, 6> &cartesian_stiffness_, double &nullspace_stiffness_, Eigen::Matrix<double, 7, 1> &q_d_nullspace_, Eigen::Matrix<double, 6, 6> &cartesian_damping_);

    // Get the state of the robot. Updates when "get_commanded_torques" is called
    void get_robot_state(Eigen::Vector3d &position_d_, Eigen::Quaterniond &orientation_d_, Eigen::Matrix<double, 6, 6> &cartesian_stiffness_, double &nullspace_stiffness_, Eigen::Matrix<double, 7, 1> &q_d_nullspace_, Eigen::Matrix<double, 6, 6> &cartesian_damping_);
    
    // Get the currently applied commands
    Eigen::VectorXd get_commands();

    // Get the jacobian
    void get_jacobian(Eigen::Matrix<double, 6, 7> &jacobian);

    // Apply a virtual Cartesian wrench
    void apply_wrench(Eigen::Matrix<double, 6, 1> cartesian_wrench);

    // Get the currently applied Cartesian wrench
    Eigen::Matrix<double, 6, 1> get_applied_wrench();

    // Saturate the torque rate of the control law
    Eigen::Matrix<double, 7, 1> saturateTorqueRate(
        const Eigen::Matrix<double, 7, 1> &tau_d_calculated,
        Eigen::Matrix<double, 7, 1> &tau_J_d, double delta_tau_max_);

    // Saturate a variable x with the limits x_min and x_max
    double saturate(double x, double x_min, double x_max);

private:
    // Robot variables

    // end effector
    Eigen::Vector3d position;
    Eigen::Vector3d position_d_;
    Eigen::Vector3d position_d_target_;

    Eigen::Quaterniond orientation;
    Eigen::Quaterniond orientation_d_;
    Eigen::Quaterniond orientation_d_target_;

    // joints &velocities
    Eigen::Matrix<double, 7, 1> q;
    Eigen::Matrix<double, 7, 1> dq;
    // jacobian
    Eigen::Matrix<double, 6, 7> jacobian;


    // Stiffness parameters
    double nullspace_stiffness_;
    double nullspace_stiffness_target_;
    Eigen::Matrix<double, 6, 6> cartesian_stiffness_;
    Eigen::Matrix<double, 6, 6> cartesian_stiffness_target_;
    Eigen::Matrix<double, 6, 6> cartesian_damping_;
    Eigen::Matrix<double, 6, 6> cartesian_damping_target_;
    Eigen::Matrix<double, 7, 1> damping_factors_;
    Eigen::Matrix<double, 7, 1> q_d_nullspace_;
    Eigen::Matrix<double, 7, 1> q_d_nullspace_target_; 
    double nullspace_damping_;
    double nullspace_damping_target_;
 
    Eigen::VectorXd tau_d;

    // Rate limiter
    Eigen::Matrix<double, 7, 1> tau_J_d_;

    // Filtering parameters   
    double update_frequency{100};
    double filter_params_stiffness{1};
    double filter_params_pose{1};
    double filter_params_wrench{1};

    //"External" applied forces
    Eigen::VectorXd tau_ext;
    Eigen::Matrix<double, 6, 1> cartesian_wrench_target_;
    Eigen::Matrix<double, 6, 1> cartesian_wrench;

    // Private functions-----

    // Update the state of the robot
    void update_states(Eigen::Matrix<double, 7, 1> q, Eigen::Matrix<double, 7, 1> dq, Eigen::Matrix<double, 6, 7> jacobian, Eigen::Vector3d position, Eigen::Quaterniond orientation, Eigen::Vector3d position_d_target_, Eigen::Quaterniond orientation_d_target_)
    {
        this->q = q;
        this->dq = dq;
        this->position << position;
        this->orientation.coeffs() << orientation.coeffs();
        this->position_d_target_ << position_d_target_;
        this->orientation_d_target_.coeffs() << orientation_d_target_.coeffs();
        this->jacobian << jacobian;
    }

    // Adds some filtering effect to stiffness
    void update_filtering_stiffness()
    {
        double filter_params_new_ = filter_params_stiffness * 100 / update_frequency;
        cartesian_stiffness_ =
            filter_params_new_ * cartesian_stiffness_target_ + (1.0 - filter_params_new_) * cartesian_stiffness_;
        cartesian_damping_ =
            filter_params_new_ * cartesian_damping_target_ + (1.0 - filter_params_new_) * cartesian_damping_;
        nullspace_stiffness_ =
            filter_params_new_ * nullspace_stiffness_target_ + (1.0 - filter_params_new_) * nullspace_stiffness_;
        q_d_nullspace_ = filter_params_new_ * q_d_nullspace_target_ + (1.0 - filter_params_new_) * q_d_nullspace_;
        nullspace_damping_=
        filter_params_new_*nullspace_damping_target_+(1.0-filter_params_new_)*nullspace_damping_;
    }

    // Adds some filtering effect to the end-effector pose
    void update_filtering_pose()
    {
        if (filter_params_pose == 1)
        {
            position_d_ << position_d_target_;
            orientation_d_.coeffs() << orientation_d_target_.coeffs();
        }
        else
        {
            double filter_params_pose_new_ = filter_params_pose * 100 / update_frequency;
            position_d_ = filter_params_pose_new_ * position_d_target_ + (1.0 - filter_params_pose_new_) * position_d_;
            orientation_d_ = orientation_d_.slerp(filter_params_pose_new_, orientation_d_target_);
        }
    }
    // Adds some filtering effect to the applied Cartesian wrench
    void update_filtering_wrench()
    {
        double filter_params_wrench_new_ = filter_params_wrench * 100 / update_frequency;
        cartesian_wrench=filter_params_wrench_new_*cartesian_wrench_target_+(1-filter_params_wrench_new_)*cartesian_wrench;
        tau_ext = jacobian.transpose() * cartesian_wrench;
    }
};