// Copyright (c) 2018 - for information on the respective copyright owner
// see the NOTICE file and/or the repository https://github.com/micro-ROS/rcl_executor.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <assert.h>
#include <iostream>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <static_executor/executors/static_single_threaded_executor.hpp>
#include <vector>


typedef struct {
    unsigned int id;
    unsigned int num_pubs;
    unsigned int num_subs;
    std::vector<int> pub_names;
    std::vector<int> sub_names;
} conf_node_t;

typedef struct {
    unsigned int rate;  // Hz
    unsigned int period;  // rate converted into ms
    unsigned int msg_size;  // number of char
    unsigned int num_nodes;
    std::vector<conf_node_t> nodes;
} conf_t;

int parse_args(int argc, const char * argv[], conf_t * conf) {
  int i = 0;
  if (argc < 4) {
    printf("Error: too few arguments missing: %d\n", 4-argc);
    printf("Usage: %s rate message_size number_nodes \
    ['node' id number_publisher number_subscriptions [topic_id]*]*\n \
    Note: for each node specify node-id, number of publishers(p) and number subscriptions(s). \n Then follows a ordered list of topic-ids. \
    The first p topic-id are topic names for the publishers, the next s topic-ids are topic name for the subscriptions.\n \
    Example 1:node 0 1 1 3 4 => node_0 with one publisher and one subscriptions \
    publisher topic='topic_3', subscription topic='topic_4'\nExample 2: node 1 2 0 3 4 => node_1 with two publishers with the topic names 'topic_3' and 'topic_4'.\n", argv[0]);
    return -1;
  }
  if (sscanf (argv[1], "%u", &conf->rate) != 1) {
    fprintf(stderr, "error - not an integer");
    return -1;
  }

  if ( conf->rate == 0) {
    printf("Error: rate must be positive.");
    return -1;
  }
  conf->period = (1000)/ conf->rate; // in milliseconds
  // conversion errors => TODO use nanoseconds, but 1000 * 1000 * 1000 is too large for unsigned int!

  if (sscanf (argv[2], "%u", &conf->msg_size) != 1) {
    fprintf(stderr, "error - not an integer");
    return -1;
  }
  if (sscanf (argv[3], "%u", &conf->num_nodes) != 1) {
    fprintf(stderr, "error - not an integer");
    return -1;
  }

  printf("Arguments: rate %d msg_size %d #nodes %d\n", conf->rate, conf->msg_size,conf->num_nodes);
  i = 4;
  for(unsigned int node_index = 0; node_index < conf->num_nodes; node_index++) {
    if ( strcmp( argv[i], "node") == 0) {
      conf_node_t node;
      i++;

      if (sscanf (argv[i], "%u", &node.id) != 1) {
        fprintf(stderr, "error - node-id not an integer");
        return -1;
      }
      i++;

      if (sscanf (argv[i], "%u", &node.num_pubs) != 1) {
        fprintf(stderr, "error - #pubs not an integer");
        return -1;
      }
      i++;

      if (sscanf (argv[i], "%u", &node.num_subs) != 1) {
        fprintf(stderr, "error - #subs not an integer");
        return -1;
      }
      i++;

      for(unsigned int p=0; p<node.num_pubs; p++) {
        unsigned int pub_name;
        if (sscanf (argv[i], "%u", &pub_name) != 1) {
          fprintf(stderr, "Error: topic_name (publisher) is not an integer");
          return -1;
        }
        node.pub_names.push_back(pub_name);
        i++;
      }
      for(unsigned int s=0; s<node.num_subs; s++) {
        unsigned int sub_name;
        if (sscanf (argv[i], "%u", &sub_name) != 1) {
          fprintf(stderr, "Error: topic_name (subscriber) is not an integer");
          return -1;
        }
        node.sub_names.push_back(sub_name);
        i++;
      }
      conf->nodes.push_back(node);

    } else {
      printf("Error: wrong arguments in node-configuration\n");
      return -1;
    }
  }
  return 0;
}

void print_configuration(conf_t * conf) {
  for(unsigned int node_index = 0; node_index < conf->num_nodes; node_index++)
  {
    printf("node %u ", conf->nodes[node_index].id);

    printf("pub: ");
    for(unsigned int i=0; i < conf->nodes[node_index].num_pubs; i++) {
      printf("%u ", conf->nodes[node_index].pub_names[i]);
    }

    printf(" sub: ");
    for(unsigned int i=0; i < conf->nodes[node_index].num_subs; i++) {
      printf("%u ", conf->nodes[node_index].sub_names[i]);
    }
    printf("\n");
  }
}


static const rclcpp::QoS qos = rclcpp::QoS(1);

/******************** MAIN PROGRAM *****************************************/
int main(int argc, const char * argv[])
{
  using namespace std::chrono_literals;
  using namespace std_msgs::msg;
  rclcpp::init(0, nullptr);
  conf_t conf;
  String msg;
  std::string node_name;
  std::string topic_name;
  rclcpp::executors::StaticSingleThreadedExecutor exec;
  std::vector<rclcpp::Node::SharedPtr> node_refs;
  std::vector<rclcpp::Subscription<String>::SharedPtr> sub_refs;
  std::vector<rclcpp::Publisher<String>::SharedPtr> pub_refs;
  std::vector<rclcpp::TimerBase::SharedPtr> timer_refs;

  if( parse_args( argc, argv, &conf) != 0) { // parse command line arguments
    printf("Error while parsing arguments.\n");
    return -1;
  }
  std::chrono::milliseconds interval(conf.period);
  print_configuration(&conf);

  for(unsigned int s=0; s < conf.msg_size; s++) // create string message with length msg_size
  {
    msg.data.append("a");
  }

  for(auto node_iter = conf.nodes.begin(); node_iter != conf.nodes.end(); node_iter++) // iterate over nodes in conf
  {
    auto node = std::make_shared<rclcpp::Node>("node_" + std::to_string(node_iter->id)); // create the node
    for(auto pub_iter = node_iter->pub_names.begin(); pub_iter != node_iter->pub_names.end(); pub_iter++) // iterate over pub_names in conf
    {
      auto pub = node->create_publisher<String>("topic_" + std::to_string(*pub_iter), qos); // create the publisher
      auto timer = node->create_wall_timer(interval, [=]() { // add timer node + publish callback
        pub->publish(msg);
        printf("Node %u: published topic_%u\n", node_iter->id, *pub_iter);
      });
      pub_refs.push_back(pub); // push back a reference to the publisher
      timer_refs.push_back(timer); // push back a reference to the timer
    }
    for( auto sub_iter = node_iter->sub_names.begin(); sub_iter != node_iter->sub_names.end(); sub_iter++) // iterate over sub_names in conf
    {
      auto sub = node->create_subscription<String>("topic_" + std::to_string(*sub_iter), qos,[=](const std_msgs::msg::String::SharedPtr msg) {
        printf("Node %u: callback: I heard: %s\n", node_iter->id, msg->data.c_str());
      });
      sub_refs.push_back(sub); // push back a reference to the subscriber
    }
    node_refs.push_back(node); // push back a referene to the node
    exec.add_node(node); // add the node to the executor
  }
  exec.spin(); // spin the executor
  rclcpp::shutdown();
  conf.nodes.clear();
  return 0;
}
