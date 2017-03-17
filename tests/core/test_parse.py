import stormpy
import stormpy.logic
from helpers.helper import get_example_path


class TestParse:
    def test_parse_prism_program(self):
        program = stormpy.parse_prism_program(get_example_path("dtmc", "die.pm"))
        assert program.nr_modules == 1
        assert program.model_type == stormpy.PrismModelType.DTMC
        assert not program.has_undefined_constants

    def test_parse_parametric_prism_program(self):
        program = stormpy.parse_prism_program(get_example_path("pdtmc", "brp16_2.pm"))
        assert program.nr_modules == 5
        assert program.model_type == stormpy.PrismModelType.DTMC
        assert program.has_undefined_constants

    def test_parse_formula(self):
        formula = "P=? [F \"one\"]"
        properties = stormpy.parse_properties(formula)
        assert len(properties) == 1
        assert str(properties[0].raw_formula) == formula
    
    def test_parse_explicit_dtmc(self):
        model = stormpy.parse_explicit_model(get_example_path("dtmc", "die.tra"), get_example_path("dtmc", "die.lab"))
        assert model.nr_states == 13
        assert model.nr_transitions == 20
        assert model.model_type == stormpy.ModelType.DTMC
        assert not model.supports_parameters
        assert type(model) is stormpy.SparseDtmc
    
    def test_parse_explicit_mdp(self):
        model = stormpy.parse_explicit_model(get_example_path("mdp", "two_dice.tra"), get_example_path("mdp", "two_dice.lab"))
        assert model.nr_states == 169
        assert model.nr_transitions == 436
        assert model.model_type == stormpy.ModelType.MDP
        assert not model.supports_parameters
        assert type(model) is stormpy.SparseMdp