#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <typeinfo>
#include "Parameter.h"
#include "GLog.h"

using namespace boost::property_tree;

// 初始化配置参数
bool Parameter::Init(const string& filepath) {
	ptree pt;

	ptree& ptNet = pt.add("Network", "");
	ptNet.add("Client.<xmlattr>.port",     portClient);
	ptNet.add("MountGWAC.<xmlattr>.port",  portMountGWAC);
	ptNet.add("Camera.<xmlattr>.port",     portCamera);
	ptNet.add("FocusGWAC.<xmlattr>.port",  portFocusGWAC);
	ptNet.add("MountGFT.<xmlattr>.port",   portMountGFT);

	ptree& ptSite = pt.add("GeoSite", "");
	ptSite.add("<xmlattr>.name", siteName);
	ptSite.add("Coords.<xmlattr>.lon", siteLon);
	ptSite.add("Coords.<xmlattr>.lat", siteLat);
	ptSite.add("Coords.<xmlattr>.alt", siteAlt);

	xml_writer_settings<std::string> settings(' ', 4);
	try {
		write_xml(filepath, pt, std::locale(), settings);
		return true;
	}
	catch(const xml_parser_error& ex) {
		_gLog.Write(LOG_FAULT, "[%s:%s]:%s", typeid(this).name(), __FUNCTION__, ex.what());
		return false;
	}
}

// 加载配置参数
bool Parameter::Load(const string& filepath) {
	try {
		ptree pt;
		read_xml(filepath, pt, xml_parser::trim_whitespace);

		portClient     = pt.get("Network.Client.<xmlattr>.port",     5010);
		portMountGWAC  = pt.get("Network.MountGWAC.<xmlattr>.port",  5011);
		portCamera     = pt.get("Network.Camera.<xmlattr>.port",     5012);
		portFocusGWAC  = pt.get("Network.FocusGWAC.<xmlattr>.port",  5013);
		portMountGFT   = pt.get("Network.MountGFT.<xmlattr>.port",   5014);

		siteName = pt.get("GeoSite.<xmlattr>.name", "");
		siteLon  = pt.get("GeoSite.Coords.<xmlattr>.lon", 120);
		siteLat  = pt.get("GeoSite.Coords.<xmlattr>.lat", 40);
		siteAlt  = pt.get("GeoSite.Coords.<xmlattr>.alt", 900);

		return true;
	}
	catch(const xml_parser_error& ex) {
		_gLog.Write(LOG_FAULT, "[%s:%s]:%s", typeid(this).name(), __FUNCTION__, ex.what());
		return false;
	}
}

// 保存配置参数
bool Parameter::Save(const string& filepath) {
	ptree pt;

	ptree& ptNet = pt.add("Network", "");
	ptNet.add("Client.<xmlattr>.port",     portClient);
	ptNet.add("MountGWAC.<xmlattr>.port",  portMountGWAC);
	ptNet.add("Camera.<xmlattr>.port",     portCamera);
	ptNet.add("FocusGWAC.<xmlattr>.port",  portFocusGWAC);
	ptNet.add("MountGFT.<xmlattr>.port",   portMountGFT);

	ptree& ptSite = pt.add("GeoSite", "");
	ptSite.add("<xmlattr>.name", siteName);
	ptSite.add("Coords.<xmlattr>.lon", siteLon);
	ptSite.add("Coords.<xmlattr>.lat", siteLat);
	ptSite.add("Coords.<xmlattr>.alt", siteAlt);

	xml_writer_settings<std::string> settings(' ', 4);
	try {
		write_xml(filepath, pt, std::locale(), settings);
		return true;
	}
	catch(const xml_parser_error& ex) {
		_gLog.Write(LOG_FAULT, "[%s:%s]:%s", typeid(this).name(), __FUNCTION__, ex.what());
		return false;
	}
}
