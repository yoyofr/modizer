#include "TreeFunctionsTest.hpp"

#ifdef _MSC_VER
#define strcasecmp stricmp
#define strncasecmp _strnicmp
#endif

prjm_eval_variable_def_t* TreeFunctions::FindVariable(const char* name)
{
    for (const auto var: m_variables)
    {
        if (strcasecmp(name, var->name) == 0)
        {
            return var;
        }
    }

    return nullptr;
}

prjm_eval_variable_def_t* TreeFunctions::CreateVariable(const char* name, PRJM_EVAL_F initialValue)
{
    auto* var = FindVariable(name);
    if (!var)
    {
        var = new prjm_eval_variable_def_t;
    }

    var->name = strdup(name);
    var->value = initialValue;

    m_variables.push_back(var);

    return var;
}

prjm_eval_exptreenode_t* TreeFunctions::CreateEmptyNode(int argCount)
{
    auto* node = reinterpret_cast<prjm_eval_exptreenode_t*>(calloc(1, sizeof(prjm_eval_exptreenode_t)));
    if (argCount > 0)
    {
        node->args = reinterpret_cast<prjm_eval_exptreenode_t**>( calloc(argCount + 1,
                                                                         sizeof(prjm_eval_exptreenode_t*)));
    }
    return node;
}

prjm_eval_exptreenode_t* TreeFunctions::CreateConstantNode(PRJM_EVAL_F value)
{
    auto* varNode = CreateEmptyNode(0);

    varNode->func = prjm_eval_func_const;
    varNode->value = value;

    return varNode;
}

prjm_eval_exptreenode_t*
TreeFunctions::CreateVariableNode(const char* name, PRJM_EVAL_F initialValue, prjm_eval_variable_def_t** variable)
{
    *variable = CreateVariable(name, initialValue);

    auto* varNode = CreateEmptyNode(0);

    varNode->func = prjm_eval_func_var;
    varNode->var = &(*variable)->value;

    return varNode;
}

void TreeFunctions::SetUp()
{
    Test::SetUp();

}

void TreeFunctions::TearDown()
{
    for (auto node: m_treeNodes)
    {
        prjm_eval_destroy_exptreenode(node);
    }

    for (const auto var: m_variables)
    {
        free(var->name); // alloc'd via C malloc/strdup!
        delete var;
    }
    m_variables.clear();

    Test::TearDown();

}


TEST_F(TreeFunctions, Constant)
{
    auto* constNode = CreateConstantNode(5.0f);

    m_treeNodes.push_back(constNode);

    PRJM_EVAL_F value{};
    PRJM_EVAL_F* valuePointer = &value;
    constNode->func(constNode, &valuePointer);

    ASSERT_EQ(*valuePointer, 5.f);
}

TEST_F(TreeFunctions, Variable)
{
    prjm_eval_variable_def_t* var;
    auto* varNode = CreateVariableNode("x", 5.f, &var);

    m_treeNodes.push_back(varNode);

    PRJM_EVAL_F value{};
    PRJM_EVAL_F* valuePointer = &value;
    varNode->func(varNode, &valuePointer);

    ASSERT_EQ(*valuePointer, 5.f);
    ASSERT_EQ(valuePointer, &var->value);
}

TEST_F(TreeFunctions, DISABLED_ExecuteList)
{

    // Expression list ("x = -50; y = 50;")
    prjm_eval_variable_def_t* var1;
    auto* varNode1 = CreateVariableNode("x", 5.f, &var1);
    auto* constNode1 = CreateConstantNode(-50.0f);

    auto* setNode1 = CreateEmptyNode(2);
    setNode1->func = prjm_eval_func_set;
    setNode1->args[0] = varNode1;
    setNode1->args[1] = constNode1;


    prjm_eval_variable_def_t* var2;
    auto* varNode2 = CreateVariableNode("y", 123.f, &var2);
    auto* constNode2 = CreateConstantNode(50.0f);

    auto* setNode2 = CreateEmptyNode(2);
    setNode2->func = prjm_eval_func_set;
    setNode2->args[0] = varNode2;
    setNode2->args[1] = constNode2;

    // Executor
    auto* listNode = CreateEmptyNode(1);
    listNode->func = prjm_eval_func_execute_list;
    listNode->args[0] = setNode1;

    m_treeNodes.push_back(listNode);

    PRJM_EVAL_F value{};
    PRJM_EVAL_F* valuePointer = &value;
    listNode->func(listNode, &valuePointer);

    ASSERT_EQ(*valuePointer, 50.f);
    ASSERT_FLOAT_EQ(var1->value, -50.f);
    ASSERT_FLOAT_EQ(var2->value, 50.f);
}

TEST_F(TreeFunctions, Set)
{
    prjm_eval_variable_def_t* var1;
    prjm_eval_variable_def_t* var2;
    auto* varNode1 = CreateVariableNode("x", 5.f, &var1);
    auto* varNode2 = CreateVariableNode("y", 45.f, &var2);

    auto* setNode = CreateEmptyNode(2);
    setNode->func = prjm_eval_func_set;
    setNode->args[0] = varNode1;
    setNode->args[1] = varNode2;

    m_treeNodes.push_back(setNode);

    PRJM_EVAL_F value{};
    PRJM_EVAL_F* valuePointer = &value;
    setNode->func(setNode, &valuePointer);

    ASSERT_EQ(valuePointer, &var1->value);
    ASSERT_NE(valuePointer, &var2->value);
    ASSERT_FLOAT_EQ(*valuePointer, 45.0f);
    ASSERT_FLOAT_EQ(var1->value, 45.0f);
}
