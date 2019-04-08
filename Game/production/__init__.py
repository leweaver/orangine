"""Production line"""
import logging
from typing import List
from typing import Dict

LOGGER = logging.getLogger(__name__)

class ProduceDefinition:
    """
    Produce is the catch all term to describe a resource that is created by some process,
    and that can be stored.

    Attributes:
        name: The internal name of this item
        displayName: The human readable name of this item
    """
    def __init__(self, name: str, displayName: str):
        self.name = name
        self.display_name = displayName

class ProduceQuantity:
    """
    Simple pairing of a Produce definition to a Quantity.
    """
    def __init__(self, produceDefinition: ProduceDefinition, quantity: int):
        self.produce_definition = produceDefinition
        self.quantity = quantity

    def take(self, take_amount: int):
        """
        Take the given takeAmount, returning a new ProduceQuantity
        """
        if take_amount > self.quantity:
            raise Exception("Taking more than you have")
        self.quantity -= take_amount
        return ProduceQuantity(self.produce_definition, take_amount)

    def __str__(self):
        return '{} ({})'.format(self.produce_definition.name, self.quantity)

ProduceQuantityList = List[ProduceQuantity]

class ProcessDefinition:
    """
    Each iteration will consume the resources given in `inputs`, and takes `iteration_work_cycles`
    number of work cycles to complete. Only once all `iteration_count` iterations are complete,
    the process creates `outputs`.

    Attributes:
        inputs: a tuple of ProduceQuantity instances.
        work: an integer representing how many work cycles are required to complete the process.
              Each work cycle requires a complete set of inputs.
        outputs: a tuple of ProduceQuantity instances.
    """
    def __init__(self, name: str, inputs: ProduceQuantityList, iteration_work_cycles: int,
                 iteration_count: int, outputs: ProduceQuantityList):
        self.name = name
        self.inputs = inputs
        self.iteration_work_cycles = iteration_work_cycles
        self.iteration_count = iteration_count
        self.outputs = outputs

class Storage:
    """
    Simple wrappper around a dict. Stores produce.

    Attributes:
        filterAllowedProduce: If not none, only allow storage of these types.
    """
    def __init__(self, max_capacity: int, filterAllowedProduce: List[ProduceDefinition] = None):
        self.max_capacity = max_capacity
        self.filter_allowed_produce = filterAllowedProduce
        self.stored_produce = {}

    def add_all(self, produce_quantity_list: List[ProduceQuantity]):
        """Adds each of the given inputs"""
        to_return_list = []
        for produce_quantity in produce_quantity_list:
            to_return = self.add(produce_quantity)
            if to_return is not None:
                to_return_list.append(to_return)
        return to_return_list

    def filter_allows(self, produce_definition: ProduceDefinition):
        """Is the given produce allowed by the filter?"""
        if self.filter_allowed_produce is None:
            return True
        for allowed_produce in self.filter_allowed_produce:
            if allowed_produce.name is produce_definition.name:
                return True
        return False

    def can_add(self, produce_quantity: ProduceQuantity):
        """Is the given produce allowed by the filter, and is there space in storage?"""
        if not self.filter_allows(produce_quantity.produce_definition):
            return False
        current = self.stored_produce.get(produce_quantity.produce_definition.name, 0)
        return current + produce_quantity.quantity <= self.max_capacity

    def add(self, produce_quantity: ProduceQuantity):
        """
        Adds as much of the given produce as possible, returning any unused.
        """
        if not self.filter_allows(produce_quantity.produce_definition):
            LOGGER.warning('Attempt to add disallowed produce: %s',
                           produce_quantity.produce_definition.name)
            return produce_quantity

        current = self.stored_produce.get(produce_quantity.produce_definition.name, 0)

        to_add = min(self.max_capacity - current, produce_quantity.quantity)
        to_return = produce_quantity.quantity - to_add
        LOGGER.debug('Adding %d quantity, returning %d', to_add, to_return)
        self.stored_produce[produce_quantity.produce_definition.name] = current + to_add

        if to_return:
            return ProduceQuantity(produce_quantity, to_return)
        return None

    def contains(self, produce_quantity: ProduceQuantity):
        """
        Returns True if there is at least as much produce in storage as requested
        """
        stored_quantity = self.stored_produce.get(produce_quantity.produce_definition.name, None)
        return stored_quantity is not None and stored_quantity >= produce_quantity.quantity

    def quantity(self, produce_definition: ProcessDefinition):
        """
        Returns the amount of the given produce currently in storage
        """
        return self.stored_produce.get(produce_definition.name, 0)

    def enforce_consume(self, produce_quantity: ProduceQuantity):
        """
        obliterates the given quantity of produce; or throws if there was insufficient in storage
        """
        stored_quantity = self.stored_produce[produce_quantity.produce_definition.name]
        if stored_quantity < produce_quantity.quantity:
            raise Exception(format("Insufficient quantity! Needed %d, but only had %d",
                                   produce_quantity.quantity, stored_quantity))
        self.stored_produce[produce_quantity.produce_definition.name] -= produce_quantity.quantity

class Producer:
    """
    This class executes a ProcessDefinition

    Attributes:
        process_definition: A ProcessDefinition instance
        progress: how many cycles have been run
        can_process: Did we consume sufficient inputs that we can make the outputs?
    """
    def __init__(self, processDefinition: ProcessDefinition, name: str = '', max_storage: int = 20):
        self.name = name
        self.process_definition = processDefinition
        self.progress = 0
        self.completed_iterations = 0
        self.can_process = False
        defns = map(lambda pq: pq.produce_definition,
                    processDefinition.inputs + processDefinition.outputs)
        self.storage = Storage(max_storage, list(defns))

    def do_work(self):
        """
        if can_process is true, adds 1 iteration of work to progress. If this
        completes the process, then Output is added to this Producers storage.
        """
        if not self.can_process:
            self._debug_log('can\'t process, starting consumption loop')
            self.can_process = self._consume_inputs()

        if not self.can_process:
            return

        self.progress += 1
        if self.progress == self.process_definition.iteration_work_cycles:
            self.completed_iterations += 1
            self.progress = 0
            self.can_process = False
            self._debug_log('completed iteration {}'.format(self.completed_iterations))

        if self.completed_iterations == self.process_definition.iteration_count:
            for output in self.process_definition.outputs:
                if not self.storage.can_add(output):
                    self._debug_log('storage of {} is full, cannot complete production'.format(
                        str(output)))
                    return

            self._debug_log('completed production')
            for output in self.process_definition.outputs:
                self.storage.add(output)

            self.completed_iterations = 0

    def _consume_inputs(self):
        for process_input in self.process_definition.inputs:
            if not self.storage.contains(process_input):
                self._debug_log('Insufficient quantity of {}. Required {}'.format(
                    process_input.produce_definition.name,
                    process_input.quantity))
                return False

        # If we got here, there are sufficient quantities of all inputs. Perform the subtraction!
        for process_input in self.process_definition.inputs:
            self._debug_log('Consuming {} of {}'.format(
                process_input.quantity,
                process_input.produce_definition.name))
            self.storage.enforce_consume(process_input)
        return True

    def _debug_log(self, msg):
        LOGGER.debug('Producer %s: %s',
                     self.process_definition.name,
                     msg)
