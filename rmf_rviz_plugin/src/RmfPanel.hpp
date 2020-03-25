/*
 * Copyright (C) 2020 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#ifndef RMF_RVIZ__PLUGIN__SRC__CONTROL_HPP
#define RMF_RVIZ__PLUGIN__SRC__CONTROL_HPP

#include <rviz_common/panel.hpp>
#include <rcl_interfaces/srv/get_parameters.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rviz_default_plugins/tools/point/point_tool.hpp>

#include <geometry_msgs/msg/point_stamped.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <geometry_msgs/msg/pose_with_covariance_stamped.hpp>
#include <rmf_dispenser_msgs/msg/dispenser_request.hpp>
#include <rmf_dispenser_msgs/msg/dispenser_state.hpp>
#include <rmf_door_msgs/msg/door_mode.hpp>
#include <rmf_door_msgs/msg/door_request.hpp>
#include <rmf_door_msgs/msg/door_state.hpp>
#include <rmf_fleet_msgs/msg/fleet_state.hpp>
#include <rmf_fleet_msgs/msg/mode_request.hpp>
#include <rmf_fleet_msgs/msg/path_request.hpp>
#include <rmf_fleet_msgs/msg/robot_mode.hpp>
#include <rmf_fleet_msgs/msg/robot_state.hpp>
#include <rmf_task_msgs/msg/delivery.hpp>
#include <rmf_task_msgs/msg/loop.hpp>
#include <rmf_task_msgs/msg/tasks.hpp>

#include <QLineEdit>
#include <QComboBox>
#include <QTimeEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QListView>
#include <QSpinBox>
#include <QTimer>
#include <QStringListModel>

#include <memory>
#include <thread>
#include <mutex>

#include "ParseGraph.hpp"

namespace rmf_rviz_plugin {

using Delivery = rmf_task_msgs::msg::Delivery;
using Loop = rmf_task_msgs::msg::Loop;
using FleetState = rmf_fleet_msgs::msg::FleetState;
using RobotState = rmf_fleet_msgs::msg::RobotState;
using DoorState = rmf_door_msgs::msg::DoorState;
using DispenserState = rmf_dispenser_msgs::msg::DispenserState;
using Location = rmf_fleet_msgs::msg::Location;
using RobotMode = rmf_fleet_msgs::msg::RobotMode;
using DoorMode = rmf_door_msgs::msg::DoorMode;
using PoseStamped = geometry_msgs::msg::PoseStamped;
using PointStamped = geometry_msgs::msg::PointStamped;
using PoseWithCovarianceStamped = geometry_msgs::msg::PoseWithCovarianceStamped;
using GetParameters = rcl_interfaces::srv::GetParameters;
using PathRequest = rmf_fleet_msgs::msg::PathRequest;
using ModeRequest = rmf_fleet_msgs::msg::ModeRequest;
using DoorRequest = rmf_door_msgs::msg::DoorRequest;
using TaskSummary = rmf_task_msgs::msg::TaskSummary;
using DispenserRequest = rmf_dispenser_msgs::msg::DispenserRequest;
using Graph = rmf_traffic::agv::Graph;

class RmfPanel : public rviz_common::Panel
{
Q_OBJECT
public:
  RmfPanel(QWidget* parent = 0);
  ~RmfPanel();

  virtual void load(const rviz_common::Config& config);
  virtual void save(rviz_common::Config config) const;

public Q_SLOTS:
  void queue_delivery();
  void queue_loop();
  void pop_delivery();
  void pop_schedule();
  void pop_loop();
  void delete_schedule_item();
  void pause_robot();
  void resume_robot();

protected Q_SLOTS:
  void update_fleet_selector();
  void update_robot_selector();
  void update_start_waypoint_selector();
  void update_end_waypoint_selector();
  void update_time_selector();
  void update_task_summary_list();
  void update_schedule();

protected:
  
  void create_layout();
  void initialize_publishers(rclcpp::Node::SharedPtr _node);
  void initialize_subscribers(rclcpp::Node::SharedPtr _node);
  void initialize_state_record();
  void initialize_qt_connections();
  void initialize_models();

  // Defining GUI QT Components - Focused on Fleets
  
  // Options - For configuring certain behaviors in the GUI
  QCheckBox* _update_time_checkbox; // If checked, update time in _time_selector
  QCheckBox* _pause_schedule_checkbox; // If checked, pause schedule running
  QCheckBox* _workcells_only_checkbox; // If checked, only, workcell waypoints will be available for selection


  // Selectors - For targeting agents to accomplish goals
  QComboBox* _fleet_selector; 
  QComboBox* _robot_selector;
  QComboBox* _start_waypoint_selector;
  QComboBox* _end_waypoint_selector;
  QSpinBox* _repeat_count_selector; // Determines number of times to repeat an action
  QTimeEdit* _time_selector;

  // Status - For visualizing important inforation on the selected agent
  QListView* _fleet_summary_view; // Displays task summaries from rmf_core
  QStringListModel* _fleet_summary_model;
  QStringList _fleet_summary_data;

  // Schedule - For visualizing and planning future schedule actions
  QListView* _schedule_list_view; // Displays [action] by [fleet] at [time]
  QPushButton* _edit_schedule_item_button;
  QPushButton* _delete_schedule_item_button;

  QStringListModel* _schedule_list_model;
  QStringList _schedule_list_data;
  std::vector<std::pair<QTime, Delivery>> _queued_deliveries;
  std::vector<std::pair<QTime, Loop>> _queued_loops;

  // Actions - For queuing commands in Schedule
  QPushButton* _send_delivery_button;
  QPushButton* _send_loop_button;
  QPushButton* _pause_robot_button;
  QPushButton* _resume_robot_button;

  // QTimer to update fields
  QTimer* _update_timer;

  bool _has_loaded = false;
  
  std::thread _thread;
  std::mutex _mutex;
  rclcpp::Node::SharedPtr _node;

private:
  // ROS2 Plumbing
  rclcpp::Subscription<FleetState>::SharedPtr _fleet_state_sub;
  rclcpp::Subscription<TaskSummary>::SharedPtr _task_summary_sub;

  rclcpp::Publisher<Delivery>::SharedPtr _delivery_pub;
  rclcpp::Publisher<Loop>::SharedPtr _loop_pub;
  rclcpp::Publisher<ModeRequest>::SharedPtr _mode_request_pub;

  // Book Keeping
  std::unordered_map<std::string, std::vector<std::string>> _map_fleet_to_robots;
  std::unordered_map<std::string, GraphInfo> _map_fleet_to_graph_info;
  std::unordered_map<std::string, RobotState> _map_robot_to_state;

  // Misc Functions
  rmf_utils::optional<GraphInfo> load_fleet_graph_info(std::string fleet_name) const;
  std::string generate_task_uuid(int len);
  bool waypoint_has_workcell(std::string waypoint_name, GraphInfo& graph_info);

  // ROS2 callbacks
  void _fleet_state_callback(const FleetState::SharedPtr msg);
  void _task_summary_callback(const TaskSummary::SharedPtr msg);

};
} // namespace rmf_rviz_plugin

#endif // RMF_RVIZ__PLUGIN__SRC__CONTROL_HPP
