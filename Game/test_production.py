"""
Unit test.
"""

import unittest
import production
from logger import init as init_logger

class Fixtures:
    """
    Test Fixtures
    """
    apple = production.ProduceDefinition("apple", "Apple")
    rhubarb = production.ProduceDefinition("rhubarb", "Rhubarb")
    sugar = production.ProduceDefinition("sugar", "Sugar")
    crumble = production.ProduceDefinition("crumble", "Apple and Rhubarb Crumble")

    @classmethod
    def test_inputs(cls):
        """Returns a list of 3 inputs"""
        return [
            production.ProduceQuantity(Fixtures.apple, 2),
            production.ProduceQuantity(Fixtures.rhubarb, 1),
            production.ProduceQuantity(Fixtures.sugar, 1)
        ]

    @classmethod
    def test_outputs(cls):
        """Returns a list of 1 expected output"""
        return [
            production.ProduceQuantity(Fixtures.crumble, 1)
        ]


class TestProducer(unittest.TestCase):
    """
    Tests Producer class
    """
    def test_single_one_cycle_iter(self):
        """A single iteration which takes 1 cycle"""
        defn = production.ProcessDefinition('Kitchen',
                                            Fixtures.test_inputs(),
                                            1,
                                            1,
                                            Fixtures.test_outputs())
        producer = production.Producer(defn)
        producer.storage.add_all([
            production.ProduceQuantity(Fixtures.apple, 2),
            production.ProduceQuantity(Fixtures.rhubarb, 1),
            production.ProduceQuantity(Fixtures.sugar, 1)
        ])

        self.assertFalse(producer.can_process)
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))
        producer.do_work()
        self.assertTrue(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))
        # Make sure that the state is reset
        self.assertFalse(producer.can_process)
        self.assertEqual(producer.progress, 0)
        self.assertEqual(producer.completed_iterations, 0)

    def test_single_two_cycle_iter(self):
        """A single iteration which takes 2 cycles"""
        defn = production.ProcessDefinition('Kitchen',
                                            Fixtures.test_inputs(),
                                            1,
                                            2,
                                            Fixtures.test_outputs())
        producer = production.Producer(defn)
        producer.storage.add_all([
            production.ProduceQuantity(Fixtures.apple, 4),
            production.ProduceQuantity(Fixtures.rhubarb, 2),
            production.ProduceQuantity(Fixtures.sugar, 2)
        ])

        self.assertFalse(producer.can_process)
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))
        producer.do_work()
        self.assertFalse(producer.can_process)
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))
        producer.do_work()
        self.assertFalse(producer.can_process)
        self.assertTrue(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))

    def test_double_one_cycle_iter(self):
        """Two iterations each taking 1 cycle"""
        defn = production.ProcessDefinition('Kitchen',
                                            Fixtures.test_inputs(),
                                            2,
                                            1,
                                            Fixtures.test_outputs())
        producer = production.Producer(defn)
        producer.storage.add_all([
            production.ProduceQuantity(Fixtures.apple, 4),
            production.ProduceQuantity(Fixtures.rhubarb, 2),
            production.ProduceQuantity(Fixtures.sugar, 2)
        ])

        self.assertFalse(producer.can_process)
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))
        producer.do_work()
        self.assertTrue(producer.can_process)
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))
        producer.do_work()
        self.assertFalse(producer.can_process)
        self.assertTrue(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))

    def test_insuffient_input_stalls(self):
        """Don't process if there is insufficient input"""
        defn = production.ProcessDefinition('Kitchen',
                                            Fixtures.test_inputs(),
                                            1,
                                            1,
                                            Fixtures.test_outputs())
        producer = production.Producer(defn)

        self.assertFalse(producer.can_process)
        producer.do_work()
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.crumble, 1)))
        self.assertEqual(producer.completed_iterations, 0)
        self.assertEqual(producer.progress, 0)

    def test_insuffient_output_storage_stalls(self):
        """Don't process if there is insufficient input"""
        defn = production.ProcessDefinition('Kitchen',
                                            Fixtures.test_inputs(),
                                            1,
                                            1,
                                            Fixtures.test_outputs())
        producer = production.Producer(defn)
        producer.storage.add_all([
            production.ProduceQuantity(Fixtures.apple, 2),
            production.ProduceQuantity(Fixtures.rhubarb, 1),
            production.ProduceQuantity(Fixtures.sugar, 1),
            production.ProduceQuantity(Fixtures.crumble, producer.storage.max_capacity),
        ])

        self.assertFalse(producer.can_process)
        producer.do_work()
        self.assertEqual(producer.completed_iterations, 1)
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.apple, 2)))
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.rhubarb, 1)))
        self.assertFalse(producer.storage.contains(production.ProduceQuantity(Fixtures.sugar, 1)))


    def test_multiple_full_loops(self):
        """Tests that the correct amount is produced and output over an extended cycle"""
        defn = production.ProcessDefinition('Kitchen',
                                            Fixtures.test_inputs(),
                                            3,
                                            2,
                                            Fixtures.test_outputs())
        producer = production.Producer(defn)

        total_produced = 0
        for i in range(60):
            # Refill the stores exactly when needed
            if i % 3 == 0:
                producer.storage.add_all([
                    production.ProduceQuantity(Fixtures.apple, 2),
                    production.ProduceQuantity(Fixtures.rhubarb, 1),
                    production.ProduceQuantity(Fixtures.sugar, 1)
                ])
            producer.do_work()

            crumble_quantity = producer.storage.quantity(Fixtures.crumble)
            if crumble_quantity > 0:
                producer.storage.enforce_consume(
                    production.ProduceQuantity(Fixtures.crumble, crumble_quantity))
                total_produced += crumble_quantity

        self.assertEqual(total_produced, 10)

if __name__ == '__main__':
    init_logger()
    unittest.main()
