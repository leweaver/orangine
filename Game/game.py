"""
Test harness for production logic
"""
import logging
import json
import logger
import production

LOGGER = logging.getLogger(__name__)

def read_produce_quantities(nodes, produce_definitions):
    if not isinstance(nodes, list):
        raise Exception('Expected list')

    produce_quantities = []
    for node in nodes:
        produce_definition = produce_definitions[node.get('produce_definition')]
        quantity = int(node.get('quantity'))
        produce_quantities.append(production.ProduceQuantity(produce_definition, quantity))

    return produce_quantities

def main():
    """
    Entry point to the python game
    """
    logger.init()

    logging.info('Loading file')
    data = json.load(open('production.json', 'r'))
    if not isinstance(data, dict):
        raise Exception('Invalid input file; expected a JSON dictionary at the root level')

    produce_definition_nodes = data.get('produce_definitions', [])
    produce_definitions = {}
    for produce_definition_node in produce_definition_nodes:
        name = produce_definition_node.get('name', None)
        if not name:
            raise Exception('Expected "name" on produce_definition')
        display_name = produce_definition_node.get('name', name)
        produce_definitions[name] = production.ProduceDefinition(name, display_name)
    LOGGER.info('Loaded %d produce_definitions', len(produce_definitions))

    process_nodes = data.get('processes', [])
    processes = {}
    for process_node in process_nodes:
        name = process_node.get('name', None)
        if not name:
            raise Exception('Expected "name" on process')
        iteration_work_cycles = int(process_node.get('iteration_work_cycles', 1))
        iteration_count = int(process_node.get('iteration_count', 1))

        inputs = read_produce_quantities(process_node.get('inputs', []), produce_definitions)
        outputs = read_produce_quantities(process_node.get('outputs', []), produce_definitions)

        processes[name] = production.ProcessDefinition(name, inputs, iteration_work_cycles,
                                                       iteration_count, outputs)
    LOGGER.info('Loaded %d processes', len(processes))

    producer_nodes = data.get('producers', [])
    producers = {}
    for producer_node in producer_nodes:
        name = producer_node.get('name', None)
        if not name:
            raise Exception('Expected "name" on producer')

        process_name = producer_node.get('process', None)
        if not process_name:
            raise Exception('Expected "process" on producer')

        process = processes[process_name]
        producers[name] = production.Producer(process, name)
    LOGGER.info('Loaded %d producers', len(producers))

if __name__ == '__main__':
    main()
