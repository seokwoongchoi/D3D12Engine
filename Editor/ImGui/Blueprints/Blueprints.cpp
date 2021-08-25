#include "stdafx.h"
#include "Blueprints.h"


#include "ImGui/NodeEditor/Include/imgui_node_editor.h"
#include "ax/Math2D.h"
#include "ax/Builders.h"
#include "ax/Widgets.h"


#include "ImGui/Source/imgui_internal.h"
//#include "Utilities/Xml.h"
//#include "ProgressBar/ProgressReport.h"

static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs)
{
	return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
}

static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs)
{
	return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
}
inline ImRect ImGui_GetItemRect()
{
	return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
}

static inline ImRect ImRect_Expanded(const ImRect& rect, float x, float y)
{
	auto result = rect;
	result.Min.x -= x;
	result.Min.y -= y;
	result.Max.x += x;
	result.Max.y += y;
	return result;
}


namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;

using namespace ax;

using ax::Widgets::IconType;

static ed::EditorContext* m_Editor = nullptr;

//extern "C" __declspec(dllimport) short __stdcall GetAsyncKeyState(int vkey);
//extern "C" bool Debug_KeyPress(int vkey)
//{
//    static std::map<int, bool> state;
//    auto lastState = state[vkey];
//    state[vkey] = (GetAsyncKeyState(vkey) & 0x8000) != 0;
//    if (state[vkey] && !lastState)
//        return true;
//    else
//        return false;
//}

enum class PinType
{
	Flow,
	Bool,
	Int,
	Float,
	String,
	Object,
	Function,
	Delegate,
};

enum class PinKind
{
	Output,
	Input
};

enum class NodeType
{
	Blueprint,
	Simple,
	Tree,
	Comment,
	Houdini
};

enum class BehaviorTreeType
{
	Root,
	Selector,
	Sequence,
	SimpleParallel,
	Condition1,
	Condition2,
	Condition3,
	Condition4,
	Action1,
	Action2,
	Action3,
	Action4,
};

struct Node;

struct Pin
{
	ed::PinId   ID;
	::Node*     Node;
	std::string Name;
	PinType     Type;
	PinKind     Kind;

	Pin(int id, const char* name, PinType type) :
		ID(id), Node(nullptr), Name(name), Type(type), Kind(PinKind::Input)
	{
	}
};


struct Node
{
	ed::NodeId ID;
	std::string Name;
	std::vector<Pin> Inputs;
	std::vector<Pin> Outputs;
	ImColor Color;
	NodeType Type;
	ImVec2 Size;

	std::string State;
	std::string SavedState;

	BehaviorTreeType bType;

	Node* Parent = nullptr;
	std::vector<Node*> Childs;

	uint index = 0;

	bool IsSecondTime = false;
	bool IsSaved = false;
	Node(int id, const char* name, ImColor color = ImColor(255, 255, 255)) :
		ID(id), Name(name), Color(color), Type(NodeType::Blueprint), Size(0, 0)
	{
		bType = BehaviorTreeType::Root;
	}
};

struct Link
{
	ed::LinkId ID;

	ed::PinId StartPinID;
	ed::PinId EndPinID;

	//::Node* startNode=nullptr;
	//::Node* EndNode = nullptr;

	ImColor Color;
	bool IsChecked = false;


	Link(ed::LinkId id, ed::PinId startPinId, ed::PinId endPinId) :
		ID(id), StartPinID(startPinId), EndPinID(endPinId), Color(255, 255, 255)
	{
	}
};


static const int            s_PinIconSize = 24;
static std::vector<Node>    s_Nodes;
static std::vector<Link>    s_Links;
//static ImTextureID          s_HeaderBackground = nullptr;
static Texture*          t_HeaderBackground = nullptr;
//static ImTextureID          s_SampleImage = nullptr;
//static ImTextureID          s_SaveIcon = nullptr;
//static ImTextureID          s_RestoreIcon = nullptr;

static Texture*          s_SaveIcon = nullptr;
static Texture*          s_RestoreIcon = nullptr;

struct NodeIdLess
{
	bool operator()(const ed::NodeId& lhs, const ed::NodeId& rhs) const
	{
		return lhs.AsPointer() < rhs.AsPointer();
	}
};

static const float          s_TouchTime = 1.0f;
static std::map<ed::NodeId, float, NodeIdLess> s_NodeTouchTime;

static int s_NextId = 1;
static int GetNextId()
{
	return s_NextId++;
}

//static ed::NodeId GetNextNodeId()
//{
//    return ed::NodeId(GetNextId());
//}

static ed::LinkId GetNextLinkId()
{
	return ed::LinkId(GetNextId());
}

static void TouchNode(ed::NodeId id)
{
	s_NodeTouchTime[id] = s_TouchTime;
}

static float GetTouchProgress(ed::NodeId id)
{
	auto it = s_NodeTouchTime.find(id);
	if (it != s_NodeTouchTime.end() && it->second > 0.0f)
		return (s_TouchTime - it->second) / s_TouchTime;
	else
		return 0.0f;
}

static void UpdateTouch()
{
	const auto deltaTime = ImGui::GetIO().DeltaTime;
	for (auto& entry : s_NodeTouchTime)
	{
		if (entry.second > 0.0f)
			entry.second -= deltaTime;
	}
}

static Node* FindNode(ed::NodeId id)
{
	for (auto& node : s_Nodes)
		if (node.ID == id)
			return &node;

	return nullptr;
}

static Link* FindLink(ed::LinkId id)
{
	for (auto& link : s_Links)
		if (link.ID == id)
			return &link;

	return nullptr;
}

static Pin* FindPin(ed::PinId id)
{
	if (!id)
		return nullptr;

	for (auto& node : s_Nodes)
	{
		for (auto& pin : node.Inputs)
			if (pin.ID == id)
				return &pin;

		for (auto& pin : node.Outputs)
			if (pin.ID == id)
				return &pin;
	}

	return nullptr;
}

static bool IsPinLinked(ed::PinId id)
{
	if (!id)
		return false;

	for (auto& link : s_Links)
		if (link.StartPinID == id || link.EndPinID == id)
			return true;

	return false;
}

static bool CanCreateLink(Pin* a, Pin* b)
{
	if (!a || !b || a == b || a->Kind == b->Kind || a->Type != b->Type || a->Node == b->Node)
		return false;

	return true;
}

//static void DrawItemRect(ImColor color, float expand = 0.0f)
//{
//    ImGui::GetWindowDrawList()->AddRect(
//        ImGui::GetItemRectMin() - ImVec2(expand, expand),
//        ImGui::GetItemRectMax() + ImVec2(expand, expand),
//        color);
//};

//static void FillItemRect(ImColor color, float expand = 0.0f, float rounding = 0.0f)
//{
//    ImGui::GetWindowDrawList()->AddRectFilled(
//        ImGui::GetItemRectMin() - ImVec2(expand, expand),
//        ImGui::GetItemRectMax() + ImVec2(expand, expand),
//        color, rounding);
//};

static void BuildNode(Node* node)
{
	for (auto& input : node->Inputs)
	{
		input.Node = node;
		input.Kind = PinKind::Input;
	}

	for (auto& output : node->Outputs)
	{
		output.Node = node;
		output.Kind = PinKind::Output;
	}
}

static Node* SpawnInputActionNode()
{
	s_Nodes.emplace_back(GetNextId(), "InputAction Fire", ImColor(255, 128, 128));
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Delegate);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Pressed", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Released", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnBranchNode()
{
	s_Nodes.emplace_back(GetNextId(), "Branch");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "True", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "False", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnDoNNode()
{
	s_Nodes.emplace_back(GetNextId(), "Do N");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Enter", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "N", PinType::Int);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Reset", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Exit", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Counter", PinType::Int);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnOutputActionNode()
{
	s_Nodes.emplace_back(GetNextId(), "OutputAction");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Sample", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Condition", PinType::Bool);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Event", PinType::Delegate);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnPrintStringNode()
{
	s_Nodes.emplace_back(GetNextId(), "Print String");
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "In String", PinType::String);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnMessageNode()
{
	s_Nodes.emplace_back(GetNextId(), "", ImColor(128, 195, 248));
	s_Nodes.back().Type = NodeType::Simple;
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Message", PinType::String);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnSetTimerNode()
{
	s_Nodes.emplace_back(GetNextId(), "Set Timer", ImColor(128, 195, 248));
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Object", PinType::Object);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Function Name", PinType::Function);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Time", PinType::Float);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Looping", PinType::Bool);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnLessNode()
{
	s_Nodes.emplace_back(GetNextId(), "<", ImColor(128, 195, 248));
	s_Nodes.back().Type = NodeType::Simple;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnWeirdNode()
{
	s_Nodes.emplace_back(GetNextId(), "o.O", ImColor(128, 195, 248));
	s_Nodes.back().Type = NodeType::Simple;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Float);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTraceByChannelNode()
{
	s_Nodes.emplace_back(GetNextId(), "Single Line Trace by Channel", ImColor(255, 128, 64));
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Start", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "End", PinType::Int);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Channel", PinType::Float);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Trace Complex", PinType::Bool);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Actors to Ignore", PinType::Int);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Draw Debug Type", PinType::Bool);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "Ignore Self", PinType::Bool);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Out Hit", PinType::Float);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "Return Value", PinType::Bool);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}
static string lastNode = "";

static Node* SpawnTreeRootNode()
{
	s_Nodes.emplace_back(GetNextId(), "Root");
	s_Nodes.back().bType = BehaviorTreeType::Root;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}
static Node* SpawnTreeSelectorNode()
{
	s_Nodes.emplace_back(GetNextId(), "Selector");
	s_Nodes.back().bType = BehaviorTreeType::Selector;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeSequenceNode()
{
	s_Nodes.emplace_back(GetNextId(), "Sequence");
	s_Nodes.back().bType = BehaviorTreeType::Sequence;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeParallelNode()
{
	s_Nodes.emplace_back(GetNextId(), "SimpleParallel");
	s_Nodes.back().bType = BehaviorTreeType::SimpleParallel;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeTaskNode()
{
	s_Nodes.emplace_back(GetNextId(), "Attack");
	s_Nodes.back().bType = BehaviorTreeType::Action1;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeTask2Node()
{
	s_Nodes.emplace_back(GetNextId(), "Patrol");
	s_Nodes.back().bType = BehaviorTreeType::Action2;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeTask3Node()
{
	s_Nodes.emplace_back(GetNextId(), "Runaway");
	s_Nodes.back().bType = BehaviorTreeType::Action3;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}
static Node* SpawnTreeTask4Node()
{
	s_Nodes.emplace_back(GetNextId(), "Strafe");
	s_Nodes.back().bType = BehaviorTreeType::Action4;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeCondition1Node()
{
	s_Nodes.emplace_back(GetNextId(), "IsSeeEnemy");
	s_Nodes.back().bType = BehaviorTreeType::Condition1;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeCondition2Node()
{
	s_Nodes.emplace_back(GetNextId(), "IsHealthLow");
	s_Nodes.back().bType = BehaviorTreeType::Condition2;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnTreeCondition3Node()
{
	s_Nodes.emplace_back(GetNextId(), "IsEnemyDead");
	s_Nodes.back().bType = BehaviorTreeType::Condition3;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}
static Node* SpawnTreeCondition4Node()
{
	s_Nodes.emplace_back(GetNextId(), "IsInRange");
	s_Nodes.back().bType = BehaviorTreeType::Condition4;
	s_Nodes.back().Type = NodeType::Tree;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	lastNode = s_Nodes.back().Name;
	s_Nodes.back().index = s_Nodes.size() - 1;
	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnComment()
{
	s_Nodes.emplace_back(GetNextId(), "Test Comment");
	s_Nodes.back().Type = NodeType::Comment;
	s_Nodes.back().Size = ImVec2(300, 200);

	return &s_Nodes.back();
}

static Node* SpawnHoudiniTransformNode()
{
	s_Nodes.emplace_back(GetNextId(), "Transform");
	s_Nodes.back().Type = NodeType::Houdini;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

static Node* SpawnHoudiniGroupNode()
{
	s_Nodes.emplace_back(GetNextId(), "Group");
	s_Nodes.back().Type = NodeType::Houdini;
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Inputs.emplace_back(GetNextId(), "", PinType::Flow);
	s_Nodes.back().Outputs.emplace_back(GetNextId(), "", PinType::Flow);

	BuildNode(&s_Nodes.back());

	return &s_Nodes.back();
}

void BuildNodes()
{
	for (auto& node : s_Nodes)
		BuildNode(&node);
}

const char* Application_GetName()
{
	return "Blueprints";
}

void Application_Initialize(ID3D12Device* device)
{
	ed::Config config;

	config.SettingsFile = "Blueprints.json";

	config.LoadNodeSettings = [](ed::NodeId nodeId, char* data, void* userPointer) -> size_t
	{
		auto node = FindNode(nodeId);
		if (!node)
			return 0;

		if (data != nullptr)
			memcpy(data, node->State.data(), node->State.size());
		return node->State.size();
	};

	config.SaveNodeSettings = [](ed::NodeId nodeId, const char* data, size_t size, ed::SaveReasonFlags reason, void* userPointer) -> bool
	{
		auto node = FindNode(nodeId);
		if (!node)
			return false;

		node->State.assign(data, size);

		TouchNode(nodeId);

		return true;
	};

	m_Editor = ed::CreateEditor(&config);
	ed::SetCurrentEditor(m_Editor);

	Node* node;
	node = SpawnTreeRootNode();      ed::SetNodePosition(node->ID, ImVec2(-252, 220));
	//node = SpawnInputActionNode();      ed::SetNodePosition(node->ID, ImVec2(-252, 220));
	//node = SpawnBranchNode();           ed::SetNodePosition(node->ID, ImVec2(-300, 351));
	//node = SpawnDoNNode();              ed::SetNodePosition(node->ID, ImVec2(-238, 504));
	//node = SpawnOutputActionNode();     ed::SetNodePosition(node->ID, ImVec2(71, 80));
	//node = SpawnSetTimerNode();         ed::SetNodePosition(node->ID, ImVec2(168, 316));

	//node = SpawnTreeSequenceNode();     ed::SetNodePosition(node->ID, ImVec2(1028, 329));
	//node = SpawnTreeTaskNode();         ed::SetNodePosition(node->ID, ImVec2(1204, 458));
	//node = SpawnTreeTask2Node();        ed::SetNodePosition(node->ID, ImVec2(868, 538));

	//node = SpawnComment();              ed::SetNodePosition(node->ID, ImVec2(112, 576));
	//node = SpawnComment();              ed::SetNodePosition(node->ID, ImVec2(800, 224));

	//node = SpawnLessNode();             ed::SetNodePosition(node->ID, ImVec2(366, 652));
	//node = SpawnWeirdNode();            ed::SetNodePosition(node->ID, ImVec2(144, 652));
	//node = SpawnMessageNode();          ed::SetNodePosition(node->ID, ImVec2(-348, 698));
	//node = SpawnPrintStringNode();      ed::SetNodePosition(node->ID, ImVec2(-69, 652));

	//node = SpawnHoudiniTransformNode(); ed::SetNodePosition(node->ID, ImVec2(500, -70));
	//node = SpawnHoudiniGroupNode();     ed::SetNodePosition(node->ID, ImVec2(500, 42));

	ed::NavigateToContent();

	BuildNodes();

	//s_Links.push_back(Link(GetNextLinkId(), s_Nodes[5].Outputs[0].ID, s_Nodes[6].Inputs[0].ID));
	//s_Links.push_back(Link(GetNextLinkId(), s_Nodes[5].Outputs[0].ID, s_Nodes[7].Inputs[0].ID));

	//s_Links.push_back(Link(GetNextLinkId(), s_Nodes[14].Outputs[0].ID, s_Nodes[15].Inputs[0].ID));


	t_HeaderBackground = new Texture();
	t_HeaderBackground->Load(device, L"BlueprintBackground.png", nullptr, true);

	//s_HeaderBackground = t_HeaderBackground->SRV();
	/*s_SaveIcon = Application_LoadTexture("Data/ic_save_white_24dp.png");
	s_RestoreIcon = Application_LoadTexture("Data/ic_restore_white_24dp.png");*/
	s_SaveIcon = new Texture(); 
	s_SaveIcon->Load(device, L"ic_save_white_24dp.png", nullptr, true);
	s_RestoreIcon = new Texture();
	s_RestoreIcon->Load(device, L"ic_restore_white_24dp.png", nullptr, true);
	//auto& io = ImGui::GetIO();
}

void Application_Finalize()
{
	auto releaseTexture = [](ImTextureID& id)
	{
		if (id)
		{
			//Application_DestroyTexture(id);
			id = nullptr;
		}
	};

	/*SafeDelete(s_RestoreIcon);
	SafeDelete(s_SaveIcon);
	SafeDelete(s_HeaderBackground);*/

	if (m_Editor)
	{
		ed::DestroyEditor(m_Editor);
		m_Editor = nullptr;
	}
}

static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
	using namespace ImGui;
	const ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImGuiID id = window->GetID("##Splitter");
	ImRect bb;
	bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
	bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
	return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

ImColor GetIconColor(PinType type)
{
	switch (type)
	{
	default:
	case PinType::Flow:     return ImColor(255, 255, 255);
	case PinType::Bool:     return ImColor(220, 48, 48);
	case PinType::Int:      return ImColor(68, 201, 156);
	case PinType::Float:    return ImColor(147, 226, 74);
	case PinType::String:   return ImColor(124, 21, 153);
	case PinType::Object:   return ImColor(51, 150, 215);
	case PinType::Function: return ImColor(218, 0, 183);
	case PinType::Delegate: return ImColor(255, 48, 48);
	}
};

void DrawPinIcon(const Pin& pin, bool connected, int alpha)
{
	IconType iconType;
	ImColor  color = GetIconColor(pin.Type);
	color.Value.w = alpha / 255.0f;
	switch (pin.Type)
	{
	case PinType::Flow:     iconType = IconType::Flow;   break;
	case PinType::Bool:     iconType = IconType::Circle; break;
	case PinType::Int:      iconType = IconType::Circle; break;
	case PinType::Float:    iconType = IconType::Circle; break;
	case PinType::String:   iconType = IconType::Circle; break;
	case PinType::Object:   iconType = IconType::Circle; break;
	case PinType::Function: iconType = IconType::Circle; break;
	case PinType::Delegate: iconType = IconType::Square; break;
	default:
		return;
	}

	ax::Widgets::Icon(ImVec2(s_PinIconSize, s_PinIconSize), iconType, connected, color, ImColor(32, 32, 32, alpha));
};

void ShowStyleEditor(bool* show = nullptr)
{
	if (!ImGui::Begin("Style", show))
	{
		ImGui::End();
		return;
	}

	auto paneWidth = ImGui::GetContentRegionAvailWidth();

	auto& editorStyle = ed::GetStyle();
	ImGui::BeginHorizontal("Style buttons", ImVec2(paneWidth, 0), 1.0f);
	ImGui::TextUnformatted("Values");
	ImGui::Spring();
	if (ImGui::Button("Reset to defaults"))
		editorStyle = ed::Style();
	ImGui::EndHorizontal();
	ImGui::Spacing();
	ImGui::DragFloat4("Node Padding", &editorStyle.NodePadding.x, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Node Rounding", &editorStyle.NodeRounding, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Node Border Width", &editorStyle.NodeBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Hovered Node Border Width", &editorStyle.HoveredNodeBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Selected Node Border Width", &editorStyle.SelectedNodeBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Pin Rounding", &editorStyle.PinRounding, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Pin Border Width", &editorStyle.PinBorderWidth, 0.1f, 0.0f, 15.0f);
	ImGui::DragFloat("Link Strength", &editorStyle.LinkStrength, 1.0f, 0.0f, 500.0f);
	//ImVec2  SourceDirection;
	//ImVec2  TargetDirection;
	ImGui::DragFloat("Scroll Duration", &editorStyle.ScrollDuration, 0.001f, 0.0f, 2.0f);
	ImGui::DragFloat("Flow Marker Distance", &editorStyle.FlowMarkerDistance, 1.0f, 1.0f, 200.0f);
	ImGui::DragFloat("Flow Speed", &editorStyle.FlowSpeed, 1.0f, 1.0f, 2000.0f);
	ImGui::DragFloat("Flow Duration", &editorStyle.FlowDuration, 0.001f, 0.0f, 5.0f);
	//ImVec2  PivotAlignment;
	//ImVec2  PivotSize;
	//ImVec2  PivotScale;
	//float   PinCorners;
	//float   PinRadius;
	//float   PinArrowSize;
	//float   PinArrowWidth;
	ImGui::DragFloat("Group Rounding", &editorStyle.GroupRounding, 0.1f, 0.0f, 40.0f);
	ImGui::DragFloat("Group Border Width", &editorStyle.GroupBorderWidth, 0.1f, 0.0f, 15.0f);

	ImGui::Separator();

	static ImGuiColorEditFlags edit_mode = ImGuiColorEditFlags_RGB;
	ImGui::BeginHorizontal("Color Mode", ImVec2(paneWidth, 0), 1.0f);
	ImGui::TextUnformatted("Filter Colors");
	ImGui::Spring();
	ImGui::RadioButton("RGB", &edit_mode, ImGuiColorEditFlags_RGB);
	ImGui::Spring(0);
	ImGui::RadioButton("HSV", &edit_mode, ImGuiColorEditFlags_HSV);
	ImGui::Spring(0);
	ImGui::RadioButton("HEX", &edit_mode, ImGuiColorEditFlags_HEX);
	ImGui::EndHorizontal();

	static ImGuiTextFilter filter;
	filter.Draw("", paneWidth);

	ImGui::Spacing();

	ImGui::PushItemWidth(-160);
	for (int i = 0; i < ed::StyleColor_Count; ++i)
	{
		auto name = ed::GetStyleColorName((ed::StyleColor)i);
		if (!filter.PassFilter(name))
			continue;

		ImGui::ColorEdit4(name, &editorStyle.Colors[i].x, edit_mode);
	}
	ImGui::PopItemWidth();

	ImGui::End();
}
void ReadNodeData(Node* node, BinaryWriter* w,bool IsEnd = false)
{

	//if (node->ID.Get() > 0&&node->Outputs)
	//{
	//	uint index = node->ID.Get() - 1;

	//	uint count = index - 1;
	//	while (false)
	//	{
	//		if (s_Nodes[count].Inputs[0].Node->ID.Get() == node->Inputs[0].Node->ID.Get())
	//		{
	//			break;
	//		}
	//		
	//	}
	//}
	//
	if (!node) return;

	switch (node->bType)
	{

	case BehaviorTreeType::Selector:
		w->String("Selector");
		
		break;
	case BehaviorTreeType::Sequence:
		w->String("Sequence");
		//cout << "Builder->Sequence()" << endl;
		break;
	case BehaviorTreeType::SimpleParallel:
		w->String("SimpleParallel");
		//cout << "Builder->Parallel(BT::EPolicy::RequireAll, BT::EPolicy::RequireOne)" << endl;
		break;
	case BehaviorTreeType::Condition1:
		w->String("Condition1");
		//cout << "Builder->Condition(BT::EConditionMode::IsSeeEnemy, false)" << endl;
		break;
	case BehaviorTreeType::Condition2:
		w->String("Condition2");
		//cout << "Builder->Condition(BT::EConditionMode::IsHealthLow, false)" << endl;
		break;
	case BehaviorTreeType::Condition3:
		w->String("Condition3");
		//cout << "Builder->Condition(BT::EConditionMode::IsEnemyDead, false)" << endl;
		break;
	case BehaviorTreeType::Condition4:
		w->String("Condition4");
		//cout << "Builder->Condition(BT::EConditionMode::IsEnemyDead, false)" << endl;
		break;
	case BehaviorTreeType::Action1:
		w->String("Action1");
		//cout << "Builder->Action(BT::EActionMode::Attack)" << endl;

		break;
	case BehaviorTreeType::Action2:
		w->String("Action2");
		//cout << "Builder->Action(BT::EActionMode::Patrol)" << endl;
		break;
	case BehaviorTreeType::Action3:
		//ut << "Builder->Action(BT::EActionMode::Runaway)" << endl;
		w->String("Action3");
		break;
	case BehaviorTreeType::Action4:
		//ut << "Builder->Action(BT::EActionMode::Runaway)" << endl;
		w->String("Action4");

		break;

	}


	ImVec2 temp=ed::GetNodePosition(node->ID);
	w->Float(temp.x);
	w->Float(temp.y);
	w->UInt(node->Parent->index);



}




void  BulidBehaviorTree(Node* node)
{
	uint outputIndex = 0;
	uint startPinId = 0;
	if (!node) return;
	if (!node->Outputs.empty())
		startPinId = node->Outputs[0].ID.Get();

	else if (node->Outputs.empty())
	{

		BulidBehaviorTree(node->Parent);
	}


	for (auto& link : s_Links)
	{
		if (link.IsChecked)
		{

			continue;
		}


		if (link.StartPinID.Get() == startPinId)
		{

			for (auto& n : s_Nodes)
			{
				if (!n.Inputs.empty())
				{


					if (n.Inputs[0].ID.Get() == link.EndPinID.Get())
					{
						outputIndex = n.ID.Get();
						n.Parent = node;
						node->Childs.emplace_back(&n);
						link.IsChecked = true;
						BulidBehaviorTree(FindNode(outputIndex));
					}

				}

			}
		}
	}
	if (node->ID.Get() != 1)
		BulidBehaviorTree(node->Parent);
	else
		return;


}


bool  RecursiveFunction(Node* node, BinaryWriter* w)
{
	uint outputIndex = 0;

	if (!node) return false;

	if (node->IsSecondTime == true)
	{

		for (uint i = 0; i < node->Childs.size(); i++)
		{
			if (node->Childs[i]->IsSecondTime == true)
			{
				continue;
			}
			outputIndex = i;
		}
		if (outputIndex == 0)
		{
			w->String("Back");
			
			RecursiveFunction(node->Parent,w);
			return false;

		}
	}
	else if (node->Childs.empty())
	{

		node->IsSecondTime = true;
		w->String("Back");
		RecursiveFunction(node->Parent,w);
		return false;

	}


	node->IsSecondTime = true;


	
	if (node->Childs[outputIndex]->Name != lastNode)
	{
		ReadNodeData(node->Childs[outputIndex],w);
		RecursiveFunction(node->Childs[outputIndex],w);
	}

	else
	{
		ReadNodeData(node->Childs[outputIndex], w,true);
		return false;
	}

	return false;






}
void BehaviorTree()
{
	if (s_Nodes.size() < 2)
		return;
	BulidBehaviorTree(&s_Nodes[0]);


	BinaryWriter* w = new BinaryWriter();
	w->Open(L"../_BehaviorTreeDatas/BehaviorTree0.behaviortree");
	w->UInt(s_Nodes.size() - 1);
	RecursiveFunction(&s_Nodes.front(),w);


	//EventSystem::Get()->CreateBuilder();
	w->Close();
	SafeDelete(w);
}

void ShowLeftPane(float paneWidth)
{

	const	auto& io = ImGui::GetIO();

	ImGui::BeginChild("Selection", ImVec2(paneWidth, 0));

	//if (ImGui::BeginMenuBar())
	//{
	//	if (ImGui::BeginMenu("File"))
	//	{
	//		if (ImGui::MenuItem("Load")) {}

	//		ImGui::Separator();

	//		if (ImGui::MenuItem("Save")) {}
	//		if (ImGui::MenuItem("Save as...")) {}

	//		ImGui::EndMenu();
	//	}


	//	ImGui::EndMenuBar();
	//}

	paneWidth = ImGui::GetContentRegionAvailWidth();

	//static bool showStyleEditor = false;
	static bool bCompiled = false;
	ImGui::BeginHorizontal("Style Editor", ImVec2(paneWidth, 0));
	ImGui::Spring(0.0f, 0.0f);
	if (ImGui::Button("Zoom to Content"))
		ed::NavigateToContent();
	ImGui::Spring(0.0f);
	if (ImGui::Button("Show Flow"))
	{
		for (auto& link : s_Links)
			ed::Flow(link.ID);
	}
	ImGui::Spring(0.0f);
	if (ImGui::Button("Load"))
	{
		HWND hWnd = NULL;
		function<void(wstring)> f = LoadAllNodes;
		Path::OpenFileDialog(L"", Path::EveryFilter, L"../_BehaviorTreeDatas/", f, hWnd);

	}
	ImGui::Spring(0.0f);
	//showStyleEditor = true;
	if (ImGui::Button("Compile"))
	{
		bCompiled = true;
	
	}

	ImGui::EndHorizontal();
	if (bCompiled)
	{
		BehaviorTree();
		bCompiled = false;
	}
	/*
	if (showStyleEditor)
		ShowStyleEditor(&showStyleEditor);
*/
	std::vector<ed::NodeId> selectedNodes;
	std::vector<ed::LinkId> selectedLinks;
	selectedNodes.resize(ed::GetSelectedObjectCount());
	selectedLinks.resize(ed::GetSelectedObjectCount());

	int nodeCount = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
	int linkCount = ed::GetSelectedLinks(selectedLinks.data(), static_cast<int>(selectedLinks.size()));

	selectedNodes.resize(nodeCount);
	selectedLinks.resize(linkCount);

	int saveIconWidth = s_SaveIcon->GetWidth();
	int saveIconHeight = s_SaveIcon->GetHeight();
	int restoreIconWidth = s_RestoreIcon->GetWidth();
	int restoreIconHeight = s_RestoreIcon->GetHeight();


	ImGui::GetWindowDrawList()->AddRectFilled(
		ImGui::GetCursorScreenPos(),
		ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
		ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
	ImGui::Spacing(); ImGui::SameLine();
	ImGui::TextUnformatted("Nodes");
	ImGui::Indent();
	for (auto& node : s_Nodes)
	{
		ImGui::PushID(node.ID.AsPointer());
		auto start = ImGui::GetCursorScreenPos();

		if (const auto progress = GetTouchProgress(node.ID))
		{
			ImGui::GetWindowDrawList()->AddLine(
				start + ImVec2(-8, 0),
				start + ImVec2(-8, ImGui::GetTextLineHeight()),
				IM_COL32(255, 0, 0, 255 - (int)(255 * progress)), 4.0f);
		}

		bool isSelected = std::find(selectedNodes.begin(), selectedNodes.end(), node.ID) != selectedNodes.end();
		if (ImGui::Selectable((node.Name + "##" + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer()))).c_str(), &isSelected))
		{
			if (io.KeyCtrl)
			{
				if (isSelected)
					ed::SelectNode(node.ID, true);
				else
					ed::DeselectNode(node.ID);
			}
			else
				ed::SelectNode(node.ID, false);

			ed::NavigateToSelection();
		}
		if (ImGui::IsItemHovered() && !node.State.empty())
			ImGui::SetTooltip("State: %s", node.State.c_str());

		auto id = std::string("(") + std::to_string(reinterpret_cast<uintptr_t>(node.ID.AsPointer())) + ")";
		auto textSize = ImGui::CalcTextSize(id.c_str(), nullptr);
		auto iconPanelPos = start + ImVec2(
			paneWidth - ImGui::GetStyle().FramePadding.x - ImGui::GetStyle().IndentSpacing - saveIconWidth - restoreIconWidth - ImGui::GetStyle().ItemInnerSpacing.x * 1,
			(ImGui::GetTextLineHeight() - saveIconHeight) / 2);
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(iconPanelPos.x - textSize.x - ImGui::GetStyle().ItemInnerSpacing.x, start.y),
			IM_COL32(255, 255, 255, 255), id.c_str(), nullptr);

		auto drawList = ImGui::GetWindowDrawList();
		ImGui::SetCursorScreenPos(iconPanelPos);
		ImGui::SetItemAllowOverlap();
		if (node.SavedState.empty())
		{
			if (ImGui::InvisibleButton("save", ImVec2((float)saveIconWidth, (float)saveIconHeight)))
				node.SavedState = node.State;

			if (ImGui::IsItemActive())
				drawList->AddImage(s_SaveIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
			else if (ImGui::IsItemHovered())
				drawList->AddImage(s_SaveIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
			else
				drawList->AddImage(s_SaveIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
		}
		else
		{
			ImGui::Dummy(ImVec2((float)saveIconWidth, (float)saveIconHeight));
			drawList->AddImage(s_SaveIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
		}

		ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
		ImGui::SetItemAllowOverlap();
		if (!node.SavedState.empty())
		{
			if (ImGui::InvisibleButton("restore", ImVec2((float)restoreIconWidth, (float)restoreIconHeight)))
			{
				node.State = node.SavedState;
				ed::RestoreNodeState(node.ID);
				node.SavedState.clear();
			}

			if (ImGui::IsItemActive())
				drawList->AddImage(s_RestoreIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 96));
			else if (ImGui::IsItemHovered())
				drawList->AddImage(s_RestoreIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
			else
				drawList->AddImage(s_RestoreIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 160));
		}
		else
		{
			ImGui::Dummy(ImVec2((float)restoreIconWidth, (float)restoreIconHeight));
			drawList->AddImage(s_RestoreIcon->SRV(), ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 32));
		}

		ImGui::SameLine(0, 0);
		ImGui::SetItemAllowOverlap();
		ImGui::Dummy(ImVec2(0, (float)restoreIconHeight));

		ImGui::PopID();
	}
	ImGui::Unindent();

	static int changeCount = 0;

	ImGui::GetWindowDrawList()->AddRectFilled(
		ImGui::GetCursorScreenPos(),
		ImGui::GetCursorScreenPos() + ImVec2(paneWidth, ImGui::GetTextLineHeight()),
		ImColor(ImGui::GetStyle().Colors[ImGuiCol_HeaderActive]), ImGui::GetTextLineHeight() * 0.25f);
	ImGui::Spacing(); ImGui::SameLine();
	ImGui::TextUnformatted("Selection");

	ImGui::BeginHorizontal("Selection Stats", ImVec2(paneWidth, 0));
	ImGui::Text("Changed %d time%s", changeCount, changeCount > 1 ? "s" : "");
	ImGui::Spring();
	if (ImGui::Button("Deselect All"))
		ed::ClearSelection();
	ImGui::EndHorizontal();
	ImGui::Indent();
	for (int i = 0; i < nodeCount; ++i) ImGui::Text("Node (%p)", selectedNodes[i].AsPointer());
	for (int i = 0; i < linkCount; ++i) ImGui::Text("Link (%p)", selectedLinks[i].AsPointer());
	ImGui::Unindent();

	if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Z)))
		for (auto& link : s_Links)
			ed::Flow(link.ID);

	if (ed::HasSelectionChanged())
		++changeCount;

	ImGui::EndChild();
}
bool CompareName(char* c, std::string s)
{
	for (uint i = 0; i < s.length(); i++)
	{

		if (c[i] != s[i]) return false;
	}

	return true;
}

void Application_Frame(bool* show)
{
	if (!ImGui::Begin("Editor", show))
	{
		ImGui::End();
		return;
	}
	UpdateTouch();

	//auto& io = ImGui::GetIO();

	//ImGui::Text("FPS: %.2f (%.2gms)", io.Framerate, io.Framerate ? 1000.0f / io.Framerate : 0.0f);

	ed::SetCurrentEditor(m_Editor);

	//auto& style = ImGui::GetStyle();


	static ed::NodeId contextNodeId = 0;
	static ed::LinkId contextLinkId = 0;
	static ed::PinId  contextPinId = 0;
	static bool createNewNode = false;
	static Pin* newNodeLinkPin = nullptr;
	static Pin* newLinkPin = nullptr;

	static float leftPaneWidth = 400.0f;
	static float rightPaneWidth = 800.0f;
	Splitter(true, 4.0f, &leftPaneWidth, &rightPaneWidth, 50.0f, 50.0f);

	ShowLeftPane(leftPaneWidth - 4.0f);

	ImGui::SameLine(0.0f, 12.0f);

	ed::Begin("Node editor");
	{
		auto cursorTopLeft = ImGui::GetCursorScreenPos();

		util::BlueprintNodeBuilder builder(t_HeaderBackground->SRV(), t_HeaderBackground->GetWidth(), t_HeaderBackground->GetHeight());

		for (auto& node : s_Nodes)
		{
			if (node.Type != NodeType::Blueprint && node.Type != NodeType::Simple)
				continue;

			const auto isSimple = node.Type == NodeType::Simple;

			bool hasOutputDelegates = false;
			for (auto& output : node.Outputs)
				if (output.Type == PinType::Delegate)
					hasOutputDelegates = true;

			builder.Begin(node.ID);
			if (!isSimple)
			{
				builder.Header(node.Color);
				ImGui::Spring(0);
				ImGui::TextUnformatted(node.Name.c_str());
				ImGui::Spring(1);
				ImGui::Dummy(ImVec2(0, 28));
				if (hasOutputDelegates)
				{
					ImGui::BeginVertical("delegates", ImVec2(0, 28));
					ImGui::Spring(1, 0);
					for (auto& output : node.Outputs)
					{
						if (output.Type != PinType::Delegate)
							continue;

						auto alpha = ImGui::GetStyle().Alpha;
						if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
							alpha = alpha * (48.0f / 255.0f);

						ed::BeginPin(output.ID, ed::PinKind::Output);
						ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
						ed::PinPivotSize(ImVec2(0, 0));
						ImGui::BeginHorizontal(output.ID.AsPointer());
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
						if (!output.Name.empty())
						{
							ImGui::TextUnformatted(output.Name.c_str());
							ImGui::Spring(0);
						}
						DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
						ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
						ImGui::EndHorizontal();
						ImGui::PopStyleVar();
						ed::EndPin();

						//DrawItemRect(ImColor(255, 0, 0));
					}
					ImGui::Spring(1, 0);
					ImGui::EndVertical();
					ImGui::Spring(0, ImGui::GetStyle().ItemSpacing.x / 2);
				}
				else
					ImGui::Spring(0);
				builder.EndHeader();
			}

			for (auto& input : node.Inputs)
			{
				auto alpha = ImGui::GetStyle().Alpha;
				if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				builder.Input(input.ID);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				DrawPinIcon(input, IsPinLinked(input.ID), (int)(alpha * 255));
				ImGui::Spring(0);
				if (!input.Name.empty())
				{
					ImGui::TextUnformatted(input.Name.c_str());
					ImGui::Spring(0);
				}
				if (input.Type == PinType::Bool)
				{
					ImGui::Button("Hello");
					ImGui::Spring(0);
				}
				ImGui::PopStyleVar();
				builder.EndInput();
			}

			if (isSimple)
			{
				builder.Middle();

				ImGui::Spring(1, 0);
				ImGui::TextUnformatted(node.Name.c_str());
				ImGui::Spring(1, 0);
			}

			for (auto& output : node.Outputs)
			{
				if (!isSimple && output.Type == PinType::Delegate)
					continue;

				auto alpha = ImGui::GetStyle().Alpha;
				if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
					alpha = alpha * (48.0f / 255.0f);

				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
				builder.Output(output.ID);
				if (output.Type == PinType::String)
				{
					static char buffer[128] = "Edit Me\nMultiline!";
					static bool wasActive = false;

					ImGui::PushItemWidth(100.0f);
					ImGui::InputText("##edit", buffer, 127);
					ImGui::PopItemWidth();
					if (ImGui::IsItemActive() && !wasActive)
					{
						ed::EnableShortcuts(false);
						wasActive = true;
					}
					else if (!ImGui::IsItemActive() && wasActive)
					{
						ed::EnableShortcuts(true);
						wasActive = false;
					}
					ImGui::Spring(0);
				}
				if (!output.Name.empty())
				{
					ImGui::Spring(0);
					ImGui::TextUnformatted(output.Name.c_str());
				}
				ImGui::Spring(0);
				DrawPinIcon(output, IsPinLinked(output.ID), (int)(alpha * 255));
				ImGui::PopStyleVar();
				builder.EndOutput();
			}

			builder.End();
		}

		for (auto& node : s_Nodes)
		{
			if (node.Type != NodeType::Tree)
				continue;
			char                InputBuf[15] = { "" };

			const float rounding = 5.0f;
			const float padding = 12.0f;

			const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

			ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(128, 128, 128, 200));
			ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(32, 32, 32, 200));
			ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(60, 180, 255, 150));
			ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(60, 180, 255, 150));

			ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
			ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
			ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
			ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
			ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
			ed::PushStyleVar(ed::StyleVar_PinRadius, 5.0f);
			ed::BeginNode(node.ID);

			ImGui::BeginVertical(node.ID.AsPointer());
			ImGui::BeginHorizontal("inputs");
			ImGui::Spring(0, padding * 2);

			ImRect inputsRect;
			int inputAlpha = 200;
			if (!node.Inputs.empty())
			{
				auto& pin = node.Inputs[0];
				ImGui::Dummy(ImVec2(0, padding));
				ImGui::Spring(1, 0);
				inputsRect = ImGui_GetItemRect();

				ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
				ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
				ed::PushStyleVar(ed::StyleVar_PinCorners, 12);
				ed::BeginPin(pin.ID, ed::PinKind::Input);
				ed::PinPivotRect(inputsRect.GetTL(), inputsRect.GetBR());
				ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
				ed::EndPin();
				ed::PopStyleVar(3);

				if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
					inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
			}
			else
				ImGui::Dummy(ImVec2(0, padding));

			ImGui::Spring(0, padding * 2);
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal("content_frame");
			ImGui::Spring(1, padding);

			ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
			ImGui::Dummy(ImVec2(160, 0));
			ImGui::Spring(1);
			for (uint i = 0; i < node.Name.length(); i++)
			{
				InputBuf[i] = node.Name[i];
			}



			ImGui::PushItemWidth(100);

			ImGui::InputText("##inputText", InputBuf, IM_ARRAYSIZE(InputBuf));

			if (!CompareName(InputBuf, node.Name))
				for (uint i = 0; i < 15; i++)
				{
					if (i > node.Name.length())break;
					node.Name[i] = InputBuf[i];
				}
			ImGui::PopItemWidth();
			//ImGui::TextUnformatted(node.Name.c_str());
			ImGui::Spring(1);
			ImGui::TextUnformatted(node.Name.c_str());
			ImGui::EndVertical();

			auto contentRect = ImGui_GetItemRect();

			ImGui::Spring(1, padding);
			ImGui::EndHorizontal();

			ImGui::BeginHorizontal("outputs");
			ImGui::Spring(0, padding * 2);

			ImRect outputsRect;
			int outputAlpha = 200;
			if (!node.Outputs.empty())
			{
				auto& pin = node.Outputs[0];
				ImGui::Dummy(ImVec2(0, padding));
				ImGui::Spring(1, 0);
				outputsRect = ImGui_GetItemRect();

				ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
				ed::BeginPin(pin.ID, ed::PinKind::Output);
				ed::PinPivotRect(outputsRect.GetTL(), outputsRect.GetBR());
				ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
				ed::EndPin();
				ed::PopStyleVar();

				if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
					outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
			}
			else
				ImGui::Dummy(ImVec2(0, padding));

			ImGui::Spring(0, padding * 2);
			ImGui::EndHorizontal();

			ImGui::EndVertical();

			ed::EndNode();
			ed::PopStyleVar(7);
			ed::PopStyleColor(4);

			auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

			//const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
			//const auto unitSize    = 1.0f / fringeScale;

			//const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
			//{
			//    if ((col >> 24) == 0)
			//        return;
			//    drawList->PathRect(a, b, rounding, rounding_corners);
			//    drawList->PathStroke(col, true, thickness);
			//};

			drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
				IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
				IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
			//ImGui::PopStyleVar();
			drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
				IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
				IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
			//ImGui::PopStyleVar();
			drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			drawList->AddRect(
				contentRect.GetTL(),
				contentRect.GetBR(),
				IM_COL32(48, 128, 255, 100), 0.0f);
			//ImGui::PopStyleVar();
		}

		for (auto& node : s_Nodes)
		{
			if (node.Type != NodeType::Houdini)
				continue;

			const float rounding = 10.0f;
			const float padding = 12.0f;


			ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(229, 229, 229, 200));
			ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(125, 125, 125, 200));
			ed::PushStyleColor(ed::StyleColor_PinRect, ImColor(229, 229, 229, 60));
			ed::PushStyleColor(ed::StyleColor_PinRectBorder, ImColor(125, 125, 125, 60));

			const auto pinBackground = ed::GetStyle().Colors[ed::StyleColor_NodeBg];

			ed::PushStyleVar(ed::StyleVar_NodePadding, ImVec4(0, 0, 0, 0));
			ed::PushStyleVar(ed::StyleVar_NodeRounding, rounding);
			ed::PushStyleVar(ed::StyleVar_SourceDirection, ImVec2(0.0f, 1.0f));
			ed::PushStyleVar(ed::StyleVar_TargetDirection, ImVec2(0.0f, -1.0f));
			ed::PushStyleVar(ed::StyleVar_LinkStrength, 0.0f);
			ed::PushStyleVar(ed::StyleVar_PinBorderWidth, 1.0f);
			ed::PushStyleVar(ed::StyleVar_PinRadius, 6.0f);
			ed::BeginNode(node.ID);

			ImGui::BeginVertical(node.ID.AsPointer());
			if (!node.Inputs.empty())
			{
				ImGui::BeginHorizontal("inputs");
				ImGui::Spring(1, 0);

				ImRect inputsRect;
				int inputAlpha = 200;
				for (auto& pin : node.Inputs)
				{
					ImGui::Dummy(ImVec2(padding, padding));
					inputsRect = ImGui_GetItemRect();
					ImGui::Spring(1, 0);
					inputsRect.Min.y -= padding;
					inputsRect.Max.y -= padding;

					//ed::PushStyleVar(ed::StyleVar_PinArrowSize, 10.0f);
					//ed::PushStyleVar(ed::StyleVar_PinArrowWidth, 10.0f);
					ed::PushStyleVar(ed::StyleVar_PinCorners, 15);
					ed::BeginPin(pin.ID, ed::PinKind::Input);
					ed::PinPivotRect(inputsRect.GetCenter(), inputsRect.GetCenter());
					ed::PinRect(inputsRect.GetTL(), inputsRect.GetBR());
					ed::EndPin();
					//ed::PopStyleVar(3);
					ed::PopStyleVar(1);

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(inputsRect.GetTL(), inputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);
					drawList->AddRect(inputsRect.GetTL(), inputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 15);

					if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
						inputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
				}

				//ImGui::Spring(1, 0);
				ImGui::EndHorizontal();
			}

			ImGui::BeginHorizontal("content_frame");
			ImGui::Spring(1, padding);

			ImGui::BeginVertical("content", ImVec2(0.0f, 0.0f));
			ImGui::Dummy(ImVec2(160, 0));
			ImGui::Spring(1);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
			ImGui::TextUnformatted(node.Name.c_str());
			ImGui::PopStyleColor();
			ImGui::Spring(1);
			ImGui::EndVertical();
			auto contentRect = ImGui_GetItemRect();

			ImGui::Spring(1, padding);
			ImGui::EndHorizontal();

			if (!node.Outputs.empty())
			{
				ImGui::BeginHorizontal("outputs");
				ImGui::Spring(1, 0);

				ImRect outputsRect;
				int outputAlpha = 200;
				for (auto& pin : node.Outputs)
				{
					ImGui::Dummy(ImVec2(padding, padding));
					outputsRect = ImGui_GetItemRect();
					ImGui::Spring(1, 0);
					outputsRect.Min.y += padding;
					outputsRect.Max.y += padding;

					ed::PushStyleVar(ed::StyleVar_PinCorners, 3);
					ed::BeginPin(pin.ID, ed::PinKind::Output);
					ed::PinPivotRect(outputsRect.GetCenter(), outputsRect.GetCenter());
					ed::PinRect(outputsRect.GetTL(), outputsRect.GetBR());
					ed::EndPin();
					ed::PopStyleVar();

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);
					drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR(),
						IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 15);


					if (newLinkPin && !CanCreateLink(newLinkPin, &pin) && &pin != newLinkPin)
						outputAlpha = (int)(255 * ImGui::GetStyle().Alpha * (48.0f / 255.0f));
				}

				ImGui::EndHorizontal();
			}

			ImGui::EndVertical();

			ed::EndNode();
			ed::PopStyleVar(7);
			ed::PopStyleColor(4);

			auto drawList = ed::GetNodeBackgroundDrawList(node.ID);

			//const auto fringeScale = ImGui::GetStyle().AntiAliasFringeScale;
			//const auto unitSize    = 1.0f / fringeScale;

			//const auto ImDrawList_AddRect = [](ImDrawList* drawList, const ImVec2& a, const ImVec2& b, ImU32 col, float rounding, int rounding_corners, float thickness)
			//{
			//    if ((col >> 24) == 0)
			//        return;
			//    drawList->PathRect(a, b, rounding, rounding_corners);
			//    drawList->PathStroke(col, true, thickness);
			//};

			//drawList->AddRectFilled(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
			//    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			//drawList->AddRect(inputsRect.GetTL() + ImVec2(0, 1), inputsRect.GetBR(),
			//    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), inputAlpha), 4.0f, 12);
			//ImGui::PopStyleVar();
			//drawList->AddRectFilled(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
			//    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
			////ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			//drawList->AddRect(outputsRect.GetTL(), outputsRect.GetBR() - ImVec2(0, 1),
			//    IM_COL32((int)(255 * pinBackground.x), (int)(255 * pinBackground.y), (int)(255 * pinBackground.z), outputAlpha), 4.0f, 3);
			////ImGui::PopStyleVar();
			//drawList->AddRectFilled(contentRect.GetTL(), contentRect.GetBR(), IM_COL32(24, 64, 128, 200), 0.0f);
			//ImGui::PushStyleVar(ImGuiStyleVar_AntiAliasFringeScale, 1.0f);
			//drawList->AddRect(
			//    contentRect.GetTL(),
			//    contentRect.GetBR(),
			//    IM_COL32(48, 128, 255, 100), 0.0f);
			//ImGui::PopStyleVar();
		}

		for (auto& node : s_Nodes)
		{
			if (node.Type != NodeType::Comment)
				continue;

			const float commentAlpha = 0.75f;

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha);
			ed::PushStyleColor(ed::StyleColor_NodeBg, ImColor(255, 255, 255, 64));
			ed::PushStyleColor(ed::StyleColor_NodeBorder, ImColor(255, 255, 255, 64));
			ed::BeginNode(node.ID);
			ImGui::PushID(node.ID.AsPointer());
			ImGui::BeginVertical("content");
			ImGui::BeginHorizontal("horizontal");
			ImGui::Spring(1);
			ImGui::TextUnformatted(node.Name.c_str());
			ImGui::Spring(1);
			ImGui::EndHorizontal();
			ed::Group(node.Size);
			ImGui::EndVertical();
			ImGui::PopID();
			ed::EndNode();
			ed::PopStyleColor(2);
			ImGui::PopStyleVar();

			if (ed::BeginGroupHint(node.ID))
			{
				//auto alpha   = static_cast<int>(commentAlpha * ImGui::GetStyle().Alpha * 255);
				auto bgAlpha = static_cast<int>(ImGui::GetStyle().Alpha * 255);

				//ImGui::PushStyleVar(ImGuiStyleVar_Alpha, commentAlpha * ImGui::GetStyle().Alpha);

				auto min = ed::GetGroupMin();
				//auto max = ed::GetGroupMax();

				ImGui::SetCursorScreenPos(min - ImVec2(-8, ImGui::GetTextLineHeightWithSpacing() + 4));
				ImGui::BeginGroup();
				ImGui::TextUnformatted(node.Name.c_str());
				ImGui::EndGroup();

				auto drawList = ed::GetHintBackgroundDrawList();

				auto hintBounds = ImGui_GetItemRect();
				auto hintFrameBounds = ImRect_Expanded(hintBounds, 8, 4);

				drawList->AddRectFilled(
					hintFrameBounds.GetTL(),
					hintFrameBounds.GetBR(),
					IM_COL32(255, 255, 255, 64 * bgAlpha / 255), 4.0f);

				drawList->AddRect(
					hintFrameBounds.GetTL(),
					hintFrameBounds.GetBR(),
					IM_COL32(255, 255, 255, 128 * bgAlpha / 255), 4.0f);

				//ImGui::PopStyleVar();
			}
			ed::EndGroupHint();
		}

		for (auto& link : s_Links)
			ed::Link(link.ID, link.StartPinID, link.EndPinID, link.Color, 2.0f);

		if (!createNewNode)
		{
			if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f))
			{
				auto showLabel = [](const char* label, ImColor color)
				{
					ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
					auto size = ImGui::CalcTextSize(label);

					auto padding = ImGui::GetStyle().FramePadding;
					auto spacing = ImGui::GetStyle().ItemSpacing;

					ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

					auto rectMin = ImGui::GetCursorScreenPos() - padding;
					auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

					auto drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
					ImGui::TextUnformatted(label);
				};

				ed::PinId startPinId = 0, endPinId = 0;
				if (ed::QueryNewLink(&startPinId, &endPinId))
				{
					auto startPin = FindPin(startPinId);
					auto endPin = FindPin(endPinId);

					newLinkPin = startPin ? startPin : endPin;

					if (startPin->Kind == PinKind::Input)
					{
						std::swap(startPin, endPin);
						std::swap(startPinId, endPinId);
					}

					if (startPin && endPin)
					{
						if (endPin == startPin)
						{
							ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						else if (endPin->Kind == startPin->Kind)
						{
							showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
							ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
						}
						//else if (endPin->Node == startPin->Node)
						//{
						//    showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
						//    ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
						//}
						else if (endPin->Type != startPin->Type)
						{
							showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
							ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
						}
						else
						{
							showLabel("+ Create Link", ImColor(32, 45, 32, 180));
							if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
							{
								s_Links.emplace_back(Link(GetNextId(), startPinId, endPinId));
								s_Links.back().Color = GetIconColor(startPin->Type);
							}
						}
					}
				}

				ed::PinId pinId = 0;
				if (ed::QueryNewNode(&pinId))
				{
					newLinkPin = FindPin(pinId);
					if (newLinkPin)
						showLabel("+ Create Node", ImColor(32, 45, 32, 180));

					if (ed::AcceptNewItem())
					{
						createNewNode = true;
						newNodeLinkPin = FindPin(pinId);
						newLinkPin = nullptr;
						ed::Suspend();
						ImGui::OpenPopup("Create New Node");
						ed::Resume();
					}
				}
			}
			else
				newLinkPin = nullptr;

			ed::EndCreate();

			if (ed::BeginDelete())
			{
				ed::LinkId linkId = 0;
				while (ed::QueryDeletedLink(&linkId))
				{
					if (ed::AcceptDeletedItem())
					{
						auto id = std::find_if(s_Links.begin(), s_Links.end(), [linkId](auto& link) { return link.ID == linkId; });
						if (id != s_Links.end())
							s_Links.erase(id);
					}
				}

				ed::NodeId nodeId = 0;
				while (ed::QueryDeletedNode(&nodeId))
				{
					if (ed::AcceptDeletedItem())
					{
						auto id = std::find_if(s_Nodes.begin(), s_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
						if (id != s_Nodes.end())
							s_Nodes.erase(id);
					}
				}
			}
			ed::EndDelete();
		}

		ImGui::SetCursorScreenPos(cursorTopLeft);
	}

# if 1
	auto openPopupPosition = ImGui::GetMousePos();
	ed::Suspend();
	if (ed::ShowNodeContextMenu(&contextNodeId))
		ImGui::OpenPopup("Node Context Menu");
	else if (ed::ShowPinContextMenu(&contextPinId))
		ImGui::OpenPopup("Pin Context Menu");
	else if (ed::ShowLinkContextMenu(&contextLinkId))
		ImGui::OpenPopup("Link Context Menu");
	else if (ed::ShowBackgroundContextMenu())
	{
		ImGui::OpenPopup("Create New Node");
		newNodeLinkPin = nullptr;
	}
	ed::Resume();

	ed::Suspend();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	if (ImGui::BeginPopup("Node Context Menu"))
	{
		auto node = FindNode(contextNodeId);

		ImGui::TextUnformatted("Node Context Menu");
		ImGui::Separator();
		if (node)
		{
			ImGui::Text("ID: %p", node->ID.AsPointer());
			ImGui::Text("Type: %s", node->Type == NodeType::Blueprint ? "Blueprint" : (node->Type == NodeType::Tree ? "Tree" : "Comment"));
			ImGui::Text("Inputs: %d", (int)node->Inputs.size());
			ImGui::Text("Outputs: %d", (int)node->Outputs.size());
		}
		else
			ImGui::Text("Unknown node: %p", contextNodeId.AsPointer());
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			ed::DeleteNode(contextNodeId);
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Pin Context Menu"))
	{
		auto pin = FindPin(contextPinId);

		ImGui::TextUnformatted("Pin Context Menu");
		ImGui::Separator();
		if (pin)
		{
			ImGui::Text("ID: %p", pin->ID.AsPointer());
			if (pin->Node)
				ImGui::Text("Node: %p", pin->Node->ID.AsPointer());
			else
				ImGui::Text("Node: %s", "<none>");
		}
		else
			ImGui::Text("Unknown pin: %p", contextPinId.AsPointer());

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Link Context Menu"))
	{
		auto link = FindLink(contextLinkId);

		ImGui::TextUnformatted("Link Context Menu");
		ImGui::Separator();
		if (link)
		{
			ImGui::Text("ID: %p", link->ID.AsPointer());
			ImGui::Text("From: %p", link->StartPinID.AsPointer());
			ImGui::Text("To: %p", link->EndPinID.AsPointer());
		}
		else
			ImGui::Text("Unknown link: %p", contextLinkId.AsPointer());
		ImGui::Separator();
		if (ImGui::MenuItem("Delete"))
			ed::DeleteLink(contextLinkId);
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("Create New Node"))
	{
		auto newNodePostion = openPopupPosition;
		//ImGui::SetCursorScreenPos(ImGui::GetMousePosOnOpeningCurrentPopup());

		//auto drawList = ImGui::GetWindowDrawList();
		//drawList->AddCircleFilled(ImGui::GetMousePosOnOpeningCurrentPopup(), 10.0f, 0xFFFF00FF);

		Node* node = nullptr;
		if (ImGui::MenuItem("Input Action"))
			node = SpawnInputActionNode();
		if (ImGui::MenuItem("Output Action"))
			node = SpawnOutputActionNode();
		if (ImGui::MenuItem("Branch"))
			node = SpawnBranchNode();
		if (ImGui::MenuItem("Do N"))
			node = SpawnDoNNode();
		if (ImGui::MenuItem("Set Timer"))
			node = SpawnSetTimerNode();
		if (ImGui::MenuItem("Less"))
			node = SpawnLessNode();
		if (ImGui::MenuItem("Weird"))
			node = SpawnWeirdNode();
		if (ImGui::MenuItem("Trace by Channel"))
			node = SpawnTraceByChannelNode();
		if (ImGui::MenuItem("Print String"))
			node = SpawnPrintStringNode();
		ImGui::Separator();
		if (ImGui::MenuItem("Comment"))
			node = SpawnComment();
		ImGui::Separator();
		//////////////////////////////////////////////////////////////
		if (ImGui::MenuItem("Selector"))
			node = SpawnTreeSelectorNode();
		if (ImGui::MenuItem("Sequence"))
			node = SpawnTreeSequenceNode();
		if (ImGui::MenuItem("Parallel"))
			node = SpawnTreeParallelNode();

		if (ImGui::MenuItem("Attack"))
			node = SpawnTreeTaskNode();
		if (ImGui::MenuItem("Patrol"))
			node = SpawnTreeTask2Node();
		if (ImGui::MenuItem("Runaway"))
			node = SpawnTreeTask3Node();
		if (ImGui::MenuItem("Strafe"))
			node = SpawnTreeTask4Node();

		if (ImGui::MenuItem("IsSeeEnemy"))
			node = SpawnTreeCondition1Node();
		if (ImGui::MenuItem("IsHealthLow"))
			node = SpawnTreeCondition2Node();
		if (ImGui::MenuItem("IsEnemyDead"))
			node = SpawnTreeCondition3Node();
		if (ImGui::MenuItem("IsInRange"))
			node = SpawnTreeCondition4Node();
		////////////////////////////////////////////////////////////////////////
		ImGui::Separator();
		if (ImGui::MenuItem("Message"))
			node = SpawnMessageNode();
		ImGui::Separator();
		if (ImGui::MenuItem("Transform"))
			node = SpawnHoudiniTransformNode();
		if (ImGui::MenuItem("Group"))
			node = SpawnHoudiniGroupNode();

		if (node)
		{
			createNewNode = false;

			ed::SetNodePosition(node->ID, newNodePostion);

			if (auto startPin = newNodeLinkPin)
			{
				auto& pins = startPin->Kind == PinKind::Input ? node->Outputs : node->Inputs;

				for (auto& pin : pins)
				{
					if (CanCreateLink(startPin, &pin))
					{
						auto endPin = &pin;
						if (startPin->Kind == PinKind::Input)
							std::swap(startPin, endPin);

						s_Links.emplace_back(Link(GetNextId(), startPin->ID, endPin->ID));
						s_Links.back().Color = GetIconColor(startPin->Type);
						break;
					}

				}
			}
		}

		ImGui::EndPopup();
	}
	else
		createNewNode = false;
	ImGui::PopStyleVar();
	ed::Resume();
# endif

	/*
		cubic_bezier_t c;
		c.p0 = pointf(100, 600);
		c.p1 = pointf(300, 1200);
		c.p2 = pointf(500, 100);
		c.p3 = pointf(900, 600);

		auto drawList = ImGui::GetWindowDrawList();
		auto offset_radius = 15.0f;
		auto acceptPoint = [drawList, offset_radius](const bezier_subdivide_result_t& r)
		{
			drawList->AddCircle(to_imvec(r.point), 4.0f, IM_COL32(255, 0, 255, 255));

			auto nt = r.tangent.normalized();
			nt = pointf(-nt.y, nt.x);

			drawList->AddLine(to_imvec(r.point), to_imvec(r.point + nt * offset_radius), IM_COL32(255, 0, 0, 255), 1.0f);
		};

		drawList->AddBezierCurve(to_imvec(c.p0), to_imvec(c.p1), to_imvec(c.p2), to_imvec(c.p3), IM_COL32(255, 255, 255, 255), 1.0f);
		cubic_bezier_subdivide(acceptPoint, c);
	*/

	ed::End();

	ImGui::End();

	//if (Keyboard::Get()->Down('H'))
	//{
	//	BulidBehaviorTree(&s_Nodes[0]);
	// 
	// int a = 0;
	//}
	//
	//ImGui::ShowTestWindow();
	//ImGui::ShowMetricsWindow();
}
//Xml::XMLElement* element = NULL;



void LoadAllNodes(const wstring & file)
{
	s_NextId = 1;
	s_Nodes.clear();
	s_Nodes.shrink_to_fit();
	s_Links.clear();
	s_Links.shrink_to_fit();

	Node* node = SpawnTreeRootNode();
	ed::SetNodePosition(node->ID, ImVec2(-252, 220));
	ed::NavigateToContent();

	BuildNodes();
	BinaryReader * r = new BinaryReader();
	r->Open(file);

	uint count = r->UInt();

	uint endPin = 1;

	for (uint i = 0; i < count; i++)
	{
		
		auto currNode = r->String();
		if (currNode == "Back")
		{
			count++;
			continue;

		}
		float x = r->Float();
		float y = r->Float();
		ImVec2 pos = ImVec2(x, y);
		uint parentIndex = r->UInt();

		if (currNode == "Selector")
		{
			node = SpawnTreeSelectorNode();

		}
		else if (currNode == "Sequence")
		{
			node = SpawnTreeSequenceNode();

		}
		else if (currNode == "SimpleParallel")
		{
			node = SpawnTreeParallelNode();

		}
		else if (currNode == "Condition1")
		{
			node = SpawnTreeCondition1Node();

		}
		else if (currNode == "Condition2")
		{
			node = SpawnTreeCondition2Node();

		}
		else if (currNode == "Condition3")
		{
			node = SpawnTreeCondition3Node();

		}
		else if (currNode == "Condition4")
		{
			node = SpawnTreeCondition4Node();

		}
		else if (currNode == "Action1")
		{
			node = SpawnTreeTaskNode();

		}
		else if (currNode == "Action2")
		{
			node = SpawnTreeTask2Node();

		}
		else if (currNode == "Action3")
		{
			node = SpawnTreeTask3Node();

		}
		else if (currNode == "Action4")
		{
			node = SpawnTreeTask4Node();

		}

		ed::SetNodePosition(node->ID, pos);
		
		s_Links.emplace_back(Link(GetNextLinkId(), s_Nodes[parentIndex].Outputs[0].ID, s_Nodes[endPin].Inputs[0].ID));
		s_Links.back().Color = GetIconColor(s_Nodes[endPin].Inputs[0].Type);

		endPin++;




	}
	r->Close();
	SafeDelete(r);
	
	ProgressReport::Get().IncrementJobsDone(ProgressReport::Model);
}

void Compile()
{
	BehaviorTree();

}



