#include <ros/ros.h>
#include <ros/package.h>
#include "dynamics/dynamics.hpp"
#include "nav_msgs/Path.h"
#include "std_msgs/Float64MultiArray.h"
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/TransformStamped.h>
#include <tf/transform_datatypes.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2/LinearMath/Quaternion.h>
#include "visualization_msgs/Marker.h"
#include "visualization_msgs/MarkerArray.h"

static nav_msgs::Path path;
static bool path_recieved;
static arma::vec s;

void path_callback(const nav_msgs::Path& msg){

    if (path.poses.size() != msg.poses.size()){
        path = msg;
        path_recieved = true;
    } else if (msg.poses.size() > 0){
        for (int i = 0; i < msg.poses.size(); i++){
            if (msg.poses[i].pose.position.x != path.poses[i].pose.position.x){
                path = msg;

                
                path_recieved = true;
                break;
            }
        }
    }   
}


void pose_callback(const visualization_msgs::Marker &msg){
  s(0) = msg.pose.position.x;
  s(1) = msg.pose.position.y;

  tf::Quaternion q(msg.pose.orientation.x, msg.pose.orientation.y, msg.pose.orientation.z, msg.pose.orientation.w);
  tf::Matrix3x3 m(q);
  double pitch, roll, yaw;
  m.getRPY(roll,pitch,yaw);
  s(2) =  yaw;

}


int main(int argc, char **argv)
{
    using std_msgs::Float64MultiArray;
    using std::string;
    using std::vector;
    using arma::vec;
    using dynamics::Q2D;
    using geometry_msgs::Pose;
    using visualization_msgs::Marker;
    using visualization_msgs::MarkerArray;


    // initialize node/node handles
    ros::init(argc, argv, "controller_node");
    ros::NodeHandle n;

    // publishers and subscribers
    ros::Publisher control_pub = n.advertise<Float64MultiArray>("/control", 10,true);
    ros::Subscriber path_sub = n.subscribe("path", 10, path_callback);
    ros::Subscriber pose_sub = n.subscribe("pose", 10, pose_callback);
    ros::Publisher plan_pub = n.advertise<MarkerArray>("/plan",10,true);

    // ROS parameters
    vector<double> x0;
    vector<double> gN;
    vector<double> gMin;
    vector<double> gMax;
    vector<double> deriv2;
    // vector<double> spat_deriv3; //should allocate memory
    // vector<double> spat_deriv4; // should allocate memory
    // vector<double> aRange;
    vector<double> pMax;
    vector<double> uMax;
    vector<double> uMin;
    // double alphaMax;
    vector<double> model_size;
    vector<double> B;
    string frame_id;

    n.getParam("x0",x0);
    // n.getParam("aRange",aRange);
    // n.getParam("alphaMax",alphaMax);
    n.getParam("pMax",pMax);
    n.getParam("gN",gN);
    n.getParam("gMin",gMin);
    n.getParam("gMax",gMax);
    n.getParam("deriv2",deriv2);
    n.getParam("uMax",uMax);
    n.getParam("uMin",uMin);
    // n.getParam("deriv4",spat_deriv4);
    // n.getParam("deriv3",spat_deriv3);
    n.getParam("frame_id",frame_id);
    n.getParam("model_size", model_size);
    n.getParam("B", B);

    // std::cout << aRange[0] << " " << aRange[1] << " " << alphaMax << std::endl;

    double max_dist = sqrt(pow(pMax[0],2) + pow(pMax[1],2));
    // std::cout << deriv2[19*80*80+7*80+71] << std::endl;

    // initialize messages
    MarkerArray pose_msg;
    Marker plan_marker;
    Marker TEB;
    plan_marker.header.frame_id = "planner";
    plan_marker.id = 0;
    plan_marker.type = 1;
    plan_marker.pose.position.x = 0;
    plan_marker.pose.position.y = 0;
    plan_marker.scale.x = model_size[0];
    plan_marker.scale.y = model_size[1];
    plan_marker.scale.z = 0.05;
    plan_marker.color.r = 0.0;
    plan_marker.color.g = 0.0;
    plan_marker.color.b = 1.0;
    plan_marker.color.a = 1.0;

    TEB.header.frame_id = "planner";
    TEB.id = 1;
    TEB.type = 1;
    TEB.pose = plan_marker.pose;
    TEB.scale.x = model_size[0] + B[0];
    TEB.scale.y = model_size[1] + B[1];
    TEB.scale.z = 0.025;
    TEB.color.r = 1.0;
    TEB.color.g = 0.0;
    TEB.color.b = 0.0;
    TEB.color.a = 0.5;
    pose_msg.markers = {plan_marker,TEB};
    tf2_ros::TransformBroadcaster br;
    geometry_msgs::TransformStamped transformStamped;

    Float64MultiArray control_msg;
    control_msg.data = {0};
    Pose planner_pose;
    vec p_goal = {0,0};
    // std::cout << 2*gN[1]*gN[2] + 1*gN[2] + 1 << std::endl;
    // std::cout << spat_deriv2[8051] << std::endl;

    // initialize model
    Q2D planner(x0[0],x0[1]);
    vec p = planner.get_state();
    s = {x0[0],x0[1],x0[2]};

    arma::mat Q = { {1,0},
                    {0,1},
                    {0,0}};

    // set publishing frequency
    int hz = 1;
    ros::Rate loop_rate(10);

    while (!path_recieved){
        ros::spinOnce();
    }

    int count = 1;
    while (ros::ok())
    {

        // Tracking Errer Block
        if (path_recieved){
            count = 1;
            planner_pose = path.poses[count].pose;  // only used in next line and for transform orientation
            p_goal = {planner_pose.position.x,planner_pose.position.y};
            path_recieved = false;
        }

        // Path Planner Block
        vec dp = p_goal - p;
        double dist = arma::norm(dp);

        
        dp *= max_dist/dist;

        p = planner.dynamics({dp(0),dp(1)},0.01);
        
        // print out planner location
        std::cout << "Planning Position:\n";
        p.print();
        std::cout << std::endl;

        transformStamped.header.stamp = ros::Time::now();
        transformStamped.header.frame_id = frame_id;
        transformStamped.child_frame_id = "planner";
        transformStamped.transform.translation.x = p(0);//planner_pose.position.x;
        transformStamped.transform.translation.y = p(1);//planner_pose.position.y;
        // transformStamped.transform.translation.z = planner_pose.position.z;
        transformStamped.transform.rotation = planner_pose.orientation; // not very functional (not updated properly)
        br.sendTransform(transformStamped);
        plan_pub.publish(pose_msg);

        // Controller Block
        vec r = s - Q*p;

        std::cout << "Relative State:\n";
        r.print();
        std::cout << std::endl;

        int r0 = round(((r(0) - gMin[0]) / (gMax[0] - gMin[0])) * gN[0]);
        int r1 = round(((r(1) - gMin[1]) / (gMax[1] - gMin[1])) * gN[1]);
        int r2 = round(((r(2) - gMin[2]) / (gMax[2] - gMin[2])) * gN[2]);
        // int r3 = round(((r(3) - gMin[3]) / (gMax[3] - gMin[3])) * gN[3]);
        // int r4 = round(((r(4) - gMin[4]) / (gMax[4] - gMin[4])) * gN[4]);

        // int r_index = r0*gN[1]*gN[2]*gN[3]*gN[4] + r1*gN[2]*gN[3]*gN[4] + r2*gN[3]*gN[4] + r3*gN[4] + r4;
        int r_index = r0*gN[1]*gN[2] + r1*gN[2] + r2;
        // int r_index = r2*gN[1]*gN[0] + r1*gN[0] + r0;

        // print indices
        std::cout << "\n" << r0 << " " << r1 << " " << r2 << " " << r_index << std::endl;
        std::cout << deriv2.at(r_index) << std::endl;

        if (deriv2.at(r_index) >= 0){
            control_msg.data[0] = uMin[0];
        } else {
            control_msg.data[0] = uMax[0];
        }

        // if (spat_deriv4[r_index] >= 0){
        //     control_msg.data[1] = -alphaMax;
        // } else {
        //     control_msg.data[1] = alphaMax;
        // }

        std::cout << "\nControl Signal:\n";
        std::cout << control_msg.data[0] << std::endl;//" " << control_msg.data[1] << std::endl;

        // Tracking Block

        control_pub.publish(control_msg);
        loop_rate.sleep();
        ros::spinOnce();
        // p = Q.t() * s;
        // planner.set_state(p);
        
        // Planning Model Block

        if (arma::norm(p_goal - p) < 0.1){
          if (++count < path.poses.size())
          {
            p_goal = {path.poses[count].pose.position.x,path.poses[count].pose.position.y};
          }
          
        }
        


        
  }


  return 0;
}