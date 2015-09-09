#include "gtest/gtest.h"
#include "storm-config.h"
#include "src/parser/PrismParser.h"
#include "src/parser/FormulaParser.h"
#include "src/logic/Formulas.h"
#include "src/permissivesched/PermissiveSchedulers.h"
#include "src/builder/ExplicitPrismModelBuilder.h"

#include "src/models/sparse/StandardRewardModel.h"


TEST(MilpPermissiveSchedulerTest, DieSelection) {
    storm::prism::Program program = storm::parser::PrismParser::parse(STORM_CPP_TESTS_BASE_PATH "/functional/builder/die_c1.nm");
    storm::parser::FormulaParser formulaParser(program.getManager().getSharedPointer());
    
    auto formula02 = formulaParser.parseSingleFormulaFromString("P>=0.10 [ F \"one\"]")->asProbabilityOperatorFormula();
    ASSERT_TRUE(storm::logic::isLowerBound(formula02.getComparisonType()));
    auto formula001 = formulaParser.parseSingleFormulaFromString("P>=0.17 [ F \"one\"]")->asProbabilityOperatorFormula();
    
    auto formula02b = formulaParser.parseSingleFormulaFromString("P<=0.10 [ F \"one\"]")->asProbabilityOperatorFormula();
    auto formula001b = formulaParser.parseSingleFormulaFromString("P<=0.17 [ F \"one\"]")->asProbabilityOperatorFormula();
    
    // Customize and perform model-building.
    typename storm::builder::ExplicitPrismModelBuilder<double>::Options options;
    
    options = typename storm::builder::ExplicitPrismModelBuilder<double>::Options(formula02);
    options.addConstantDefinitionsFromString(program, "");
    options.buildCommandLabels = true;
    
    std::shared_ptr<storm::models::sparse::Mdp<double>> mdp = storm::builder::ExplicitPrismModelBuilder<double>::translateProgram(program, options)->as<storm::models::sparse::Mdp<double>>();
    
     boost::optional<storm::ps::MemorylessDeterministicPermissiveScheduler> perms = storm::ps::computePermissiveSchedulerViaMILP(mdp, formula02);
     EXPECT_NE(perms, boost::none);
     boost::optional<storm::ps::MemorylessDeterministicPermissiveScheduler> perms2 = storm::ps::computePermissiveSchedulerViaMILP(mdp, formula001);
     EXPECT_EQ(perms2, boost::none);
     
     boost::optional<storm::ps::MemorylessDeterministicPermissiveScheduler> perms3 = storm::ps::computePermissiveSchedulerViaMILP(mdp, formula02b);
     EXPECT_EQ(perms3, boost::none);
     boost::optional<storm::ps::MemorylessDeterministicPermissiveScheduler> perms4 = storm::ps::computePermissiveSchedulerViaMILP(mdp, formula001b);
     EXPECT_NE(perms4, boost::none);
 // 
    
}