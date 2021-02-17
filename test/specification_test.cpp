// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include "adapter/adapter.hpp"
#include "agent.hpp"
#include "agent_test_helper.hpp"
#include "json_helper.hpp"
#include "device_model/specifications.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <set>

using json = nlohmann::json;
using namespace std;
using namespace mtconnect;

class SpecificationTest : public testing::Test
{
 protected:
  void SetUp() override
  {  // Create an agent with only 16 slots and 8 data items.
    m_agentTestHelper = make_unique<AgentTestHelper>();
    m_agentTestHelper->createAgent("/samples/configuration.xml",
                                   8, 4, "1.7", 25);
    auto device = m_agentTestHelper->m_agent->getDeviceByName("LinuxCNC");
    m_component = device->getComponentById("c");
  }

  void TearDown() override
  {
    m_agentTestHelper.reset();
  }

  adapter::Adapter *m_adapter{nullptr};
  Component *m_component{nullptr};
  std::unique_ptr<AgentTestHelper> m_agentTestHelper;
};

TEST_F(SpecificationTest, ParseDeviceAndComponentRelationships)
{
  ASSERT_NE(nullptr, m_component);

  auto &configurations_list = m_component->getConfiguration();

  ASSERT_TRUE(!configurations_list.empty());

  auto configurations_entity = configurations_list.front()->getEntity();

  auto &configuration = configurations_entity->get<entity::EntityList>("LIST");

  ASSERT_EQ(2, configuration.size());

  auto config = configuration.begin();

  config++;

  EXPECT_EQ("Specifications", (*config)->getName());

  auto &specs = (*config)->get<entity::EntityList>("LIST");

  ASSERT_EQ(3, specs.size());
  
  auto it = specs.begin();

  EXPECT_EQ("spec", (*it)->get<string>("id"));
  EXPECT_EQ("ROTARY_VELOCITY", (*it)->get<string>("type"));
  EXPECT_EQ("ACTUAL", (*it)->get<string>("subType"));
  EXPECT_EQ("REVOLUTION/MINUTE", (*it)->get<string>("units"));
  EXPECT_EQ("speed_limit", (*it)->get<string>("name"));
  EXPECT_EQ("cmotor", (*it)->get<string>("compositionIdRef"));
  EXPECT_EQ("machine", (*it)->get<string>("coordinateSystemIdRef"));
  EXPECT_EQ("c1", (*it)->get<string>("dataItemIdRef"));
  EXPECT_EQ("Specification", (*it)->getName());

  EXPECT_EQ(10000.0, get<double>((*it)->getProperty("Maximum")));
  EXPECT_EQ(100.0, get<double>((*it)->getProperty("Minimum")));
  EXPECT_EQ(1000.0, get<double>((*it)->getProperty("Nominal")));
}

/*
#define CONFIGURATION_PATH "//m:Rotary[@id='c']/m:Configuration"
#define SPECIFICATIONS_PATH CONFIGURATION_PATH "/m:Specifications"

TEST_F(SpecificationTest, XmlPrinting)
{
  {
    PARSE_XML_RESPONSE("/LinuxCNC/probe");
    
    ASSERT_XML_PATH_COUNT(doc, SPECIFICATIONS_PATH , 1);
    ASSERT_XML_PATH_COUNT(doc, SPECIFICATIONS_PATH "/*" , 3);

    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']@type" , "ROTARY_VELOCITY");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']@subType" , "ACTUAL");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']@units" , "REVOLUTION/MINUTE");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']@compositionIdRef" , "cmotor");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']@coordinateSystemIdRef" , "machine");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']@dataItemIdRef" , "c1");

    ASSERT_XML_PATH_COUNT(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']/*", 3);
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']/m:Maximum" , "10000");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']/m:Minimum" , "100");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@name='speed_limit']/m:Nominal" , "1000");
  }
}

TEST_F(SpecificationTest, XmlPrintingForLoadSpec)
{
  {
    PARSE_XML_RESPONSE("/LinuxCNC/probe");
    
    // Load spec
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']@type" , "LOAD");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']@units" , "PERCENT");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']@name" , "loadspec");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']@originator" , "MANUFACTURER");
    
    ASSERT_XML_PATH_COUNT(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/*", 7);
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/m:Maximum" , "1000");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/m:Minimum" , "-1000");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/m:Nominal" , "100");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/m:UpperLimit" , "500");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/m:LowerLimit" , "-500");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/m:UpperWarning" , "200");
    ASSERT_XML_PATH_EQUAL(doc, SPECIFICATIONS_PATH "/m:Specification[@id='spec1']/m:LowerWarning" , "-200");
  }
}


TEST_F(SpecificationTest, JsonPrinting)
{
  m_agentTestHelper->m_request.m_accepts = "Application/json";
  
  {
    PARSE_JSON_RESPONSE("/LinuxCNC/probe");
    
    auto devices = doc.at("/MTConnectDevices/Devices"_json_pointer);
    auto device = devices.at(0).at("/Device"_json_pointer);

    auto rotary = device.at("/Components/0/Axes/Components/0/Rotary"_json_pointer);
    auto specifications = rotary.at("/Configuration/Specifications"_json_pointer);
    ASSERT_TRUE(specifications.is_array());
    ASSERT_EQ(3_S, specifications.size());

    auto crel = specifications.at(0);
    auto cfields = crel.at("/Specification"_json_pointer);
    EXPECT_EQ("ROTARY_VELOCITY", cfields["type"]);
    EXPECT_EQ("ACTUAL", cfields["subType"]);
    EXPECT_EQ("REVOLUTION/MINUTE", cfields["units"]);
    EXPECT_EQ("speed_limit", cfields["name"]);
    EXPECT_EQ("cmotor", cfields["compositionIdRef"]);
    EXPECT_EQ("machine", cfields["coordinateSystemIdRef"]);
    EXPECT_EQ("c1", cfields["dataItemIdRef"]);
    
    EXPECT_EQ(10000.0, cfields["Maximum"]);
    EXPECT_EQ(100.0, cfields["Minimum"]);
    EXPECT_EQ(1000.0, cfields["Nominal"]);
  }
}

TEST_F(SpecificationTest, JsonPrintingForLoadSpec)
{
  m_agentTestHelper->m_request.m_accepts = "Application/json";

  {
    PARSE_JSON_RESPONSE("/LinuxCNC/probe");
    
    auto devices = doc.at("/MTConnectDevices/Devices"_json_pointer);
    auto device = devices.at(0).at("/Device"_json_pointer);
    
    auto rotary = device.at("/Components/0/Axes/Components/0/Rotary"_json_pointer);
    auto specifications = rotary.at("/Configuration/Specifications"_json_pointer);
    ASSERT_TRUE(specifications.is_array());
    ASSERT_EQ(3_S, specifications.size());
    
    auto crel = specifications.at(1);
    auto cfields = crel.at("/Specification"_json_pointer);
    EXPECT_EQ("spec1", cfields["id"]);
    EXPECT_EQ("LOAD", cfields["type"]);
    EXPECT_EQ("PERCENT", cfields["units"]);
    EXPECT_EQ("loadspec", cfields["name"]);
    EXPECT_EQ("MANUFACTURER", cfields["originator"]);

    EXPECT_EQ(1000.0, cfields["Maximum"]);
    EXPECT_EQ(-1000.0, cfields["Minimum"]);
    EXPECT_EQ(100.0, cfields["Nominal"]);
    EXPECT_EQ(500.0, cfields["UpperLimit"]);
    EXPECT_EQ(-500.0, cfields["LowerLimit"]);
    EXPECT_EQ(200, cfields["UpperWarning"]);
    EXPECT_EQ(-200, cfields["LowerWarning"]);
  }
}


TEST_F(SpecificationTest, Parse17SpecificationValues)
{
  ASSERT_NE(nullptr, m_component);
  
  ASSERT_EQ(2, m_component->getConfiguration().size());
  
  auto ci = m_component->getConfiguration().begin();
  // Get the second configuration.
  ci++;
  const auto conf = ci->get();
  ASSERT_EQ(typeid(Specifications), typeid(*conf));
  
  const auto specs = dynamic_cast<const Specifications*>(conf);
  ASSERT_NE(nullptr, specs);
  ASSERT_EQ(3, specs->getSpecifications().size());
  
  auto si = specs->getSpecifications().begin();
  
  // Advance to second specificaiton
  si++;

  EXPECT_EQ("Specification", (*si)->getClass());

  EXPECT_EQ("spec1", (*si)->m_id);
  EXPECT_EQ("LOAD", (*si)->m_type);
  EXPECT_EQ("PERCENT", (*si)->m_units);
  EXPECT_EQ("loadspec", (*si)->m_name);
  EXPECT_EQ("MANUFACTURER", (*si)->m_originator);
  
  EXPECT_FALSE((*si)->hasGroups());

  EXPECT_EQ(1000.0, (*si)->getLimit("Maximum"));
  EXPECT_EQ(-1000.0, (*si)->getLimit("Minimum"));
  EXPECT_EQ(100.0, (*si)->getLimit("Nominal"));
  EXPECT_EQ(500.0, (*si)->getLimit("UpperLimit"));
  EXPECT_EQ(-500.0, (*si)->getLimit("LowerLimit"));
  EXPECT_EQ(200.0, (*si)->getLimit("UpperWarning"));
  EXPECT_EQ(-200.0, (*si)->getLimit("LowerWarning"));
}

TEST_F(SpecificationTest, ParseProcessSpecificationValues)
{
  ASSERT_NE(nullptr, m_component);
  
  ASSERT_EQ(2, m_component->getConfiguration().size());
  
  auto ci = m_component->getConfiguration().begin();
  // Get the second configuration.
  ci++;
  const auto conf = ci->get();
  ASSERT_EQ(typeid(Specifications), typeid(*conf));
  
  const auto specs = dynamic_cast<const Specifications*>(conf);
  ASSERT_NE(nullptr, specs);
  ASSERT_EQ(3, specs->getSpecifications().size());
  
  auto si = specs->getSpecifications().begin();
  
  // Advance to third specificaiton
  si++; si++;
  EXPECT_EQ("ProcessSpecification", (*si)->getClass());
  
  EXPECT_EQ("pspec1", (*si)->m_id);
  EXPECT_EQ("LOAD", (*si)->m_type);
  EXPECT_EQ("PERCENT", (*si)->m_units);
  EXPECT_EQ("procspec", (*si)->m_name);
  EXPECT_EQ("USER", (*si)->m_originator);
  
  EXPECT_TRUE((*si)->hasGroups());
  auto groups = (*si)->getGroupKeys();
  std::set<string> expected = { "SpecificationLimits", "AlarmLimits", "ControlLimits" };
  ASSERT_EQ(expected, groups);
  
  const auto spec = (*si)->getGroup("SpecificationLimits");
  EXPECT_TRUE(spec);
  
  EXPECT_EQ(500.0, spec->find("UpperLimit")->second);
  EXPECT_EQ(50.0, spec->find("Nominal")->second);
  EXPECT_EQ(-500.0, spec->find("LowerLimit")->second);

  const auto control = (*si)->getGroup("ControlLimits");
  EXPECT_TRUE(control);
  
  EXPECT_EQ(500.0, control->find("UpperLimit")->second);
  EXPECT_EQ(200.0, control->find("UpperWarning")->second);
  EXPECT_EQ(10, control->find("Nominal")->second);
  EXPECT_EQ(-200.0, control->find("LowerWarning")->second);
  EXPECT_EQ(-500.0, control->find("LowerLimit")->second);

  const auto alarm = (*si)->getGroup("ControlLimits");
  EXPECT_TRUE(alarm);
  
  EXPECT_EQ(500.0, alarm->find("UpperLimit")->second);
  EXPECT_EQ(200.0, alarm->find("UpperWarning")->second);
  EXPECT_EQ(-200.0, alarm->find("LowerWarning")->second);
  EXPECT_EQ(-500.0, alarm->find("LowerLimit")->second);
}

#define CONFIGURATION_PATH "//m:Rotary[@id='c']/m:Configuration"
#define PROCESS_PATH CONFIGURATION_PATH "/m:Specifications/m:ProcessSpecification"


TEST_F(SpecificationTest, XmlPrintingForProcessSpecification)
{
  {
    PARSE_XML_RESPONSE("/LinuxCNC/probe");
    
    ASSERT_XML_PATH_COUNT(doc, PROCESS_PATH "/*" , 3);
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "@id" , "pspec1");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "@type" , "LOAD");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "@units" , "PERCENT");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "@originator" , "USER");

    ASSERT_XML_PATH_COUNT(doc, PROCESS_PATH "/m:SpecificationLimits/*" , 3);
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:SpecificationLimits/m:UpperLimit" , "500");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:SpecificationLimits/m:LowerLimit" , "-500");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:SpecificationLimits/m:Nominal" , "50");

    ASSERT_XML_PATH_COUNT(doc, PROCESS_PATH "/m:ControlLimits/*" , 5);
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:ControlLimits/m:UpperLimit" , "500");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:ControlLimits/m:LowerLimit" , "-500");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:ControlLimits/m:UpperWarning" , "200");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:ControlLimits/m:LowerWarning" , "-200");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:ControlLimits/m:Nominal" , "10");

    ASSERT_XML_PATH_COUNT(doc, PROCESS_PATH "/m:AlarmLimits/*" , 4);
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:AlarmLimits/m:UpperLimit" , "500");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:AlarmLimits/m:LowerLimit" , "-500");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:AlarmLimits/m:UpperWarning" , "200");
    ASSERT_XML_PATH_EQUAL(doc, PROCESS_PATH "/m:AlarmLimits/m:LowerWarning" , "-200");
  }
}

TEST_F(SpecificationTest, JsonPrintingForProcessSpecification)
{
  m_agentTestHelper->m_request.m_accepts = "Application/json";
  
  {
    PARSE_JSON_RESPONSE("/LinuxCNC/probe");
    
    auto devices = doc.at("/MTConnectDevices/Devices"_json_pointer);
    auto device = devices.at(0).at("/Device"_json_pointer);
    
    auto rotary = device.at("/Components/0/Axes/Components/0/Rotary"_json_pointer);
    auto specifications = rotary.at("/Configuration/Specifications"_json_pointer);
    ASSERT_TRUE(specifications.is_array());
    ASSERT_EQ(3_S, specifications.size());
    
    auto crel = specifications.at(2);
    auto cfields = crel.at("/ProcessSpecification"_json_pointer);
    EXPECT_EQ("pspec1", cfields["id"]);
    EXPECT_EQ("LOAD", cfields["type"]);
    EXPECT_EQ("PERCENT", cfields["units"]);
    EXPECT_EQ("procspec", cfields["name"]);
    EXPECT_EQ("USER", cfields["originator"]);
    
    auto specs = cfields["SpecificationLimits"];
    EXPECT_EQ(500.0, specs["UpperLimit"]);
    EXPECT_EQ(50.0, specs["Nominal"]);
    EXPECT_EQ(-500.0, specs["LowerLimit"]);

    auto control = cfields["ControlLimits"];
    EXPECT_EQ(500.0, control["UpperLimit"]);
    EXPECT_EQ(10.0, control["Nominal"]);
    EXPECT_EQ(-500.0, control["LowerLimit"]);
    EXPECT_EQ(200.0, control["UpperWarning"]);
    EXPECT_EQ(-200.0, control["LowerWarning"]);
    
    auto alarm = cfields["AlarmLimits"];
    EXPECT_EQ(500.0, alarm["UpperLimit"]);
    EXPECT_EQ(-500.0, alarm["LowerLimit"]);
    EXPECT_EQ(200.0, alarm["UpperWarning"]);
    EXPECT_EQ(-200.0, alarm["LowerWarning"]);
  }
}
*/