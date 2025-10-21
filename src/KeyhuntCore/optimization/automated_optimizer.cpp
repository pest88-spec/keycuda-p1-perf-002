// Puzzle71Solver - Automated Performance Optimization Implementation (T046)
// Phase 6: User Story 4 - Performance Monitoring
// AI-driven parameter tuning and automated performance optimization

#include "automated_optimizer.cuh"
#include "utils/logger.h"
#include "utils/json_serializer.h"
#include "compute/gpu/separated_kernel_executor.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <random>
#include <filesystem>

namespace puzzle71::optimization {

// OptimizationResult implementation
double OptimizationResult::calculateImprovement(const OptimizationResult& baseline) const {
    if (baseline.objective_value == 0.0) {
        return 0.0;
    }
    return ((objective_value - baseline.objective_value) / std::abs(baseline.objective_value)) * 100.0;
}

std::string OptimizationResult::toJson() const {
    std::ostringstream json;
    json << std::fixed << std::setprecision(4);

    json << "{\n";
    json << "  \"success\": " << (success ? "true" : "false") << ",\n";
    json << "  \"objective_value\": " << objective_value << ",\n";
    json << "  \"optimization_time_ms\": " << optimization_time.count() << ",\n";
    json << "  \"iterations_completed\": " << iterations_completed << ",\n";
    json << "  \"algorithm_used\": \"" << algorithm_used << "\",\n";
    json << "  \"optimization_notes\": \"" << optimization_notes << "\",\n";

    json << "  \"parameter_values\": {\n";
    bool first = true;
    for (const auto& [name, value] : parameter_values) {
        if (!first) json << ",\n";
        json << "    \"" << name << "\": \"" << value << "\"";
        first = false;
    }
    json << "\n  },\n";

    json << "  \"objective_history\": [";
    for (size_t i = 0; i < objective_history.size(); ++i) {
        if (i > 0) json << ", ";
        json << objective_history[i];
    }
    json << "],\n";

    json << "  \"performance_metrics\": " << performance_metrics.toJson() << "\n";
    json << "}";

    return json.str();
}

OptimizationResult OptimizationResult::fromJson(const std::string& json) {
    // Simplified JSON parsing - integrate with existing JSON serializer
    OptimizationResult result;

    // This is a placeholder for full JSON parsing
    // In production, use the existing JSON serialization utilities
    try {
        // Parse basic fields
        auto success_pos = json.find("\"success\":");
        if (success_pos != std::string::npos) {
            result.success = json.substr(success_pos + 10, 4) == "true";
        }

        auto objective_pos = json.find("\"objective_value\":");
        if (objective_pos != std::string::npos) {
            auto end_pos = json.find(",", objective_pos);
            auto obj_str = json.substr(objective_pos + 18, end_pos - objective_pos - 18);
            result.objective_value = std::stod(obj_str);
        }

        // Parse other fields similarly...

    } catch (const std::exception& e) {
        Logger::error("Failed to parse OptimizationResult from JSON: {}", e.what());
    }

    return result;
}

// OptimizationConstraints implementation
bool OptimizationConstraints::satisfiesConstraints(
    const puzzle71::monitoring::PerformanceMetrics& metrics,
    const std::map<std::string, std::string>& parameters) const {

    // Performance constraints
    if (min_throughput > 0 && metrics.keys_per_second < min_throughput) {
        return false;
    }

    if (max_latency_ms < std::numeric_limits<double>::max() &&
        metrics.kernel_execution_time_ms > max_latency_ms) {
        return false;
    }

    if (max_power_watts < std::numeric_limits<double>::max() &&
        metrics.power_usage_watts > max_power_watts) {
        return false;
    }

    if (max_memory_mb < std::numeric_limits<double>::max() &&
        metrics.memory_used_bytes > max_memory_mb * 1024 * 1024) {
        return false;
    }

    // Hardware constraints
    auto grid_size_it = parameters.find("grid_size");
    if (grid_size_it != parameters.end()) {
        auto grid_size = std::stoul(grid_size_it->second);
        if (grid_size > max_grid_size) {
            return false;
        }
    }

    auto block_size_it = parameters.find("block_size");
    if (block_size_it != parameters.end()) {
        auto block_size = std::stoul(block_size_it->second);
        if (block_size > max_block_size) {
            return false;
        }
    }

    return true;
}

// GridSearchOptimizer implementation
GridSearchOptimizer::GridSearchOptimizer(size_t samples_per_dimension)
    : samples_per_dimension_(samples_per_dimension) {}

void GridSearchOptimizer::initialize(
    const std::vector<OptimizationParameter>& parameters,
    const OptimizationConstraints& constraints,
    OptimizationObjective objective) {

    parameters_ = parameters;
    constraints_ = constraints;
    objective_ = objective;

    generateParameterGrid();

    current_indices_.resize(parameter_values_.size(), 0);
    enumeration_complete_ = false;

    Logger::info("Initialized Grid Search optimizer with {} parameters, {} samples per dimension",
                 parameters.size(), samples_per_dimension_);
}

std::map<std::string, std::string> GridSearchOptimizer::getNextParameters() {
    if (enumeration_complete_) {
        return {};
    }

    auto current_params = getCurrentParameters();
    advanceToNextCombination();

    return current_params;
}

void GridSearchOptimizer::updateResults(
    const std::map<std::string, std::string>& parameters,
    double objective_value,
    const puzzle71::monitoring::PerformanceMetrics& metrics) {
    // Grid search doesn't use feedback - it's systematic
    Logger::trace("Grid search evaluated point with objective: {}", objective_value);
}

bool GridSearchOptimizer::isOptimizationComplete(const OptimizationState& state) const {
    return enumeration_complete_ ||
           state.current_iteration >= constraints_.max_iterations ||
           (std::chrono::steady_clock::now() - state.start_time) >= constraints_.max_optimization_time;
}

std::map<std::string, double> GridSearchOptimizer::getStatistics() const {
    return {
        {"total_combinations", static_cast<double>(parameter_values_.size() ?
            std::accumulate(parameter_values_.begin(), parameter_values_.end(), 1ULL,
                [](size_t acc, const auto& vec) { return acc * vec.size(); }) : 0)},
        {"current_combination", static_cast<double>(std::accumulate(
            current_indices_.begin(), current_indices_.end(), 0ULL) + 1)},
        {"completion_percentage", enumeration_complete_ ? 100.0 :
            (static_cast<double>(std::accumulate(current_indices_.begin(), current_indices_.end(), 0ULL) + 1) * 100.0 /
             std::accumulate(parameter_values_.begin(), parameter_values_.end(), 1ULL,
                [](size_t acc, const auto& vec) { return acc * vec.size(); }))}
    };
}

void GridSearchOptimizer::generateParameterGrid() {
    parameter_values_.clear();

    for (const auto& param : parameters_) {
        std::vector<std::string> values;

        switch (param.type) {
            case ParameterType::INTEGER: {
                int step = (param.max_int - param.min_int) / static_cast<int>(samples_per_dimension_ - 1);
                for (int i = 0; i < static_cast<int>(samples_per_dimension_); ++i) {
                    int value = param.min_int + i * step;
                    values.push_back(std::to_string(value));
                }
                break;
            }

            case ParameterType::FLOAT: {
                double step = (param.max_float - param.min_float) / (samples_per_dimension_ - 1);
                for (size_t i = 0; i < samples_per_dimension_; ++i) {
                    double value = param.min_float + i * step;
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(4) << value;
                    values.push_back(oss.str());
                }
                break;
            }

            case ParameterType::BOOLEAN: {
                values.push_back("false");
                values.push_back("true");
                break;
            }

            case ParameterType::CATEGORICAL: {
                values = param.categories;
                break;
            }
        }

        parameter_values_.push_back(values);
    }

    Logger::debug("Generated parameter grid with {} dimensions", parameter_values_.size());
}

std::map<std::string, std::string> GridSearchOptimizer::getCurrentParameters() {
    std::map<std::string, std::string> params;

    for (size_t i = 0; i < parameters_.size() && i < current_indices_.size(); ++i) {
        if (current_indices_[i] < parameter_values_[i].size()) {
            params[parameters_[i].name] = parameter_values_[i][current_indices_[i]];
        }
    }

    return params;
}

void GridSearchOptimizer::advanceToNextCombination() {
    for (int i = static_cast<int>(current_indices_.size()) - 1; i >= 0; --i) {
        current_indices_[i]++;

        if (current_indices_[i] < parameter_values_[i].size()) {
            return;
        }

        current_indices_[i] = 0;
    }

    enumeration_complete_ = true;
}

// BayesianOptimizer implementation
BayesianOptimizer::BayesianOptimizer(size_t initial_samples)
    : initial_samples_(initial_samples), random_generator_(std::random_device{}()) {}

void BayesianOptimizer::initialize(
    const std::vector<OptimizationParameter>& parameters,
    const OptimizationConstraints& constraints,
    OptimizationObjective objective) {

    parameters_ = parameters;
    constraints_ = constraints;
    objective_ = objective;
    samples_.clear();
    initial_sampling_complete_ = false;

    Logger::info("Initialized Bayesian optimizer with {} initial samples", initial_samples_);
}

std::map<std::string, std::string> BayesianOptimizer::getNextParameters() {
    std::map<std::string, std::string> parameters;

    if (!initial_sampling_complete_) {
        generateInitialSample(parameters);
        return parameters;
    }

    if (samples_.size() >= constraints_.max_iterations) {
        return {};
    }

    parameters = generateCandidateFromSurrogate();
    return parameters;
}

void BayesianOptimizer::updateResults(
    const std::map<std::string, std::string>& parameters,
    double objective_value,
    const puzzle71::monitoring::PerformanceMetrics& metrics) {

    SamplePoint sample;
    sample.parameters = parameters;
    sample.objective_value = objective_value;
    sample.metrics = metrics;

    samples_.push_back(sample);

    if (samples_.size() >= initial_samples_) {
        initial_sampling_complete_ = true;
        Logger::debug("Bayesian optimizer completed initial sampling phase");
    }

    Logger::trace("Bayesian optimizer updated with objective value: {}", objective_value);
}

bool BayesianOptimizer::isOptimizationComplete(const OptimizationState& state) const {
    return samples_.size() >= constraints_.max_iterations ||
           (std::chrono::steady_clock::now() - state.start_time) >= constraints_.max_optimization_time ||
           (initial_sampling_complete_ && state.iterations_without_improvement > 20);
}

std::map<std::string, double> BayesianOptimizer::getStatistics() const {
    return {
        {"samples_collected", static_cast<double>(samples_.size())},
        {"initial_sampling_complete", initial_sampling_complete_ ? 1.0 : 0.0},
        {"exploration_exploitation_balance", initial_sampling_complete_ ? 0.7 : 0.3}
    };
}

void BayesianOptimizer::generateInitialSample(std::map<std::string, std::string>& parameters) {
    std::uniform_real_distribution<double> float_dist(0.0, 1.0);
    std::uniform_int_distribution<int> int_dist(0, 100);

    for (const auto& param : parameters_) {
        switch (param.type) {
            case ParameterType::INTEGER: {
                int value = param.min_int + int_dist(random_generator_) %
                           (param.max_int - param.min_int + 1);
                parameters[param.name] = std::to_string(value);
                break;
            }

            case ParameterType::FLOAT: {
                double value = param.min_float + float_dist(random_generator_) *
                              (param.max_float - param.min_float);
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(4) << value;
                parameters[param.name] = oss.str();
                break;
            }

            case ParameterType::BOOLEAN: {
                parameters[param.name] = (int_dist(random_generator_) % 2 == 0) ? "true" : "false";
                break;
            }

            case ParameterType::CATEGORICAL: {
                if (!param.categories.empty()) {
                    size_t index = int_dist(random_generator_) % param.categories.size();
                    parameters[param.name] = param.categories[index];
                }
                break;
            }
        }
    }
}

std::map<std::string, std::string> BayesianOptimizer::generateCandidateFromSurrogate() {
    // Simplified surrogate model - would use Gaussian Process in production
    std::map<std::string, std::string> best_params;
    double best_acquisition = std::numeric_limits<double>::lowest();

    // Generate multiple candidates and pick the best based on acquisition function
    for (int i = 0; i < 50; ++i) {
        std::map<std::string, std::string> candidate;
        generateInitialSample(candidate);

        double acquisition_value = evaluateAcquisitionFunction(candidate);
        if (acquisition_value > best_acquisition) {
            best_acquisition = acquisition_value;
            best_params = candidate;
        }
    }

    return best_params;
}

double BayesianOptimizer::evaluateAcquisitionFunction(const std::map<std::string, std::string>& parameters) {
    // Simplified Upper Confidence Bound (UCB) acquisition function
    double mean = 0.0;
    double variance = 1.0;  // Simplified - would use GP prediction in production

    if (!samples_.empty()) {
        // Find nearest samples and estimate mean
        double total_weight = 0.0;
        double weighted_sum = 0.0;

        auto param_vec = parametersToVector(parameters);

        for (const auto& sample : samples_) {
            auto sample_vec = parametersToVector(sample.parameters);

            // Calculate Euclidean distance
            double distance = 0.0;
            for (size_t i = 0; i < std::min(param_vec.size(), sample_vec.size()); ++i) {
                distance += (param_vec[i] - sample_vec[i]) * (param_vec[i] - sample_vec[i]);
            }
            distance = std::sqrt(distance);

            // Weight by inverse distance
            double weight = 1.0 / (1.0 + distance);
            weighted_sum += weight * sample.objective_value;
            total_weight += weight;
        }

        if (total_weight > 0) {
            mean = weighted_sum / total_weight;
            variance = 1.0 / (1.0 + total_weight);  // Uncertainty decreases with more samples
        }
    }

    // UCB = mean + exploration_factor * sqrt(variance)
    double exploration_factor = 2.0;
    return mean + exploration_factor * std::sqrt(variance);
}

std::vector<double> BayesianOptimizer::parametersToVector(const std::map<std::string, std::string>& parameters) {
    std::vector<double> vector;

    for (const auto& param : parameters_) {
        auto it = parameters.find(param.name);
        if (it != parameters.end()) {
            switch (param.type) {
                case ParameterType::INTEGER:
                    vector.push_back(static_cast<double>(std::stoi(it->second)));
                    break;
                case ParameterType::FLOAT:
                    vector.push_back(std::stod(it->second));
                    break;
                case ParameterType::BOOLEAN:
                    vector.push_back(it->second == "true" ? 1.0 : 0.0);
                    break;
                case ParameterType::CATEGORICAL: {
                    auto cat_it = std::find(param.categories.begin(), param.categories.end(), it->second);
                    size_t index = std::distance(param.categories.begin(), cat_it);
                    vector.push_back(static_cast<double>(index));
                    break;
                }
            }
        } else {
            vector.push_back(0.0);  // Default value
        }
    }

    return vector;
}

std::map<std::string, std::string> BayesianOptimizer::vectorToParameters(const std::vector<double>& vector) {
    std::map<std::string, std::string> parameters;

    for (size_t i = 0; i < parameters_.size() && i < vector.size(); ++i) {
        const auto& param = parameters_[i];

        switch (param.type) {
            case ParameterType::INTEGER: {
                int value = static_cast<int>(std::round(vector[i]));
                value = std::max(param.min_int, std::min(param.max_int, value));
                parameters[param.name] = std::to_string(value);
                break;
            }

            case ParameterType::FLOAT: {
                double value = vector[i];
                value = std::max(param.min_float, std::min(param.max_float, value));
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(4) << value;
                parameters[param.name] = oss.str();
                break;
            }

            case ParameterType::BOOLEAN: {
                parameters[param.name] = (vector[i] > 0.5) ? "true" : "false";
                break;
            }

            case ParameterType::CATEGORICAL: {
                size_t index = static_cast<size_t>(std::round(vector[i]));
                index = std::min(index, param.categories.size() - 1);
                parameters[param.name] = param.categories[index];
                break;
            }
        }
    }

    return parameters;
}

// GeneticOptimizer implementation
GeneticOptimizer::GeneticOptimizer(size_t population_size, double mutation_rate, double crossover_rate)
    : population_size_(population_size), mutation_rate_(mutation_rate), crossover_rate_(crossover_rate),
      random_generator_(std::random_device{}()) {}

void GeneticOptimizer::initialize(
    const std::vector<OptimizationParameter>& parameters,
    const OptimizationConstraints& constraints,
    OptimizationObjective objective) {

    parameters_ = parameters;
    constraints_ = constraints;
    objective_ = objective;

    initializePopulation();
    current_individual_ = 0;
    generation_ = 0;

    Logger::info("Initialized Genetic Algorithm with population size {}, mutation rate {:.3f}, crossover rate {:.3f}",
                 population_size_, mutation_rate_, crossover_rate_);
}

std::map<std::string, std::string> GeneticOptimizer::getNextParameters() {
    if (current_individual_ >= population_.size()) {
        // Evolution step
        selection();
        crossover();
        mutation();
        current_individual_ = 0;
        generation_++;
    }

    if (current_individual_ < population_.size()) {
        auto params = population_[current_individual_].parameters;
        current_individual_++;
        return params;
    }

    return {};
}

void GeneticOptimizer::updateResults(
    const std::map<std::string, std::string>& parameters,
    double objective_value,
    const puzzle71::monitoring::PerformanceMetrics& metrics) {

    // Find the individual that matches these parameters
    for (auto& individual : population_) {
        bool matches = true;
        for (const auto& [name, value] : parameters) {
            if (individual.parameters[name] != value) {
                matches = false;
                break;
            }
        }

        if (matches) {
            individual.fitness = objective_value;
            individual.metrics = metrics;
            break;
        }
    }

    Logger::trace("Genetic algorithm updated individual with fitness: {}", objective_value);
}

bool GeneticOptimizer::isOptimizationComplete(const OptimizationState& state) const {
    return generation_ >= 100 ||  // Max generations
           (std::chrono::steady_clock::now() - state.start_time) >= constraints_.max_optimization_time ||
           state.iterations_without_improvement > 20;
}

std::map<std::string, double> GeneticOptimizer::getStatistics() const {
    double avg_fitness = 0.0;
    double max_fitness = std::numeric_limits<double>::lowest();

    if (!population_.empty()) {
        for (const auto& individual : population_) {
            avg_fitness += individual.fitness;
            max_fitness = std::max(max_fitness, individual.fitness);
        }
        avg_fitness /= population_.size();
    }

    return {
        {"generation", static_cast<double>(generation_)},
        {"population_size", static_cast<double>(population_.size())},
        {"average_fitness", avg_fitness},
        {"best_fitness", max_fitness},
        {"current_individual", static_cast<double>(current_individual_)}
    };
}

void GeneticOptimizer::initializePopulation() {
    population_.clear();
    population_.reserve(population_size_);

    for (size_t i = 0; i < population_size_; ++i) {
        population_.push_back(createRandomIndividual());
    }

    Logger::debug("Initialized genetic algorithm population with {} individuals", population_size_);
}

void GeneticOptimizer::selection() {
    // Tournament selection
    std::vector<Individual> selected;
    selected.reserve(population_size_);

    std::uniform_int_distribution<int> tournament_dist(0, population_size_ - 1);
    int tournament_size = std::max(3, static_cast<int>(population_size_ / 10));

    for (size_t i = 0; i < population_size_; ++i) {
        Individual best = population_[tournament_dist(random_generator_)];

        for (int j = 1; j < tournament_size; ++j) {
            Individual candidate = population_[tournament_dist(random_generator_)];
            if (candidate.fitness > best.fitness) {
                best = candidate;
            }
        }

        selected.push_back(best);
    }

    population_ = std::move(selected);
}

void GeneticOptimizer::crossover() {
    offspring_.clear();
    offspring_.reserve(population_size_);

    std::uniform_real_distribution<double> crossover_dist(0.0, 1.0);
    std::uniform_int_distribution<int> parent_dist(0, population_size_ - 1);

    for (size_t i = 0; i < population_size_; i += 2) {
        Individual parent1 = population_[parent_dist(random_generator_)];
        Individual parent2 = population_[parent_dist(random_generator_)];

        if (crossover_dist(random_generator_) < crossover_rate_) {
            Individual child1 = crossoverIndividuals(parent1, parent2);
            Individual child2 = crossoverIndividuals(parent2, parent1);
            offspring_.push_back(child1);
            offspring_.push_back(child2);
        } else {
            offspring_.push_back(parent1);
            offspring_.push_back(parent2);
        }
    }

    // Ensure we have exactly population_size offspring
    while (offspring_.size() < population_size_) {
        offspring_.push_back(createRandomIndividual());
    }

    offspring_.resize(population_size_);
}

void GeneticOptimizer::mutation() {
    population_ = std::move(offspring_);

    for (auto& individual : population_) {
        mutateIndividual(individual);
    }
}

Individual GeneticOptimizer::createRandomIndividual() {
    Individual individual;
    individual.parameters = getRandomParameters();
    individual.fitness = 0.0;
    return individual;
}

Individual GeneticOptimizer::crossoverIndividuals(const Individual& parent1, const Individual& parent2) {
    Individual child;
    child.parameters = parent1.parameters;

    std::uniform_real_distribution<double> crossover_dist(0.0, 1.0);

    for (const auto& [name, value] : parent2.parameters) {
        if (crossover_dist(random_generator_) < 0.5) {
            child.parameters[name] = value;
        }
    }

    child.fitness = 0.0;
    return child;
}

void GeneticOptimizer::mutateIndividual(Individual& individual) {
    std::uniform_real_distribution<double> mutation_dist(0.0, 1.0);

    for (const auto& param : parameters_) {
        if (mutation_dist(random_generator_) < mutation_rate_) {
            // Mutate this parameter
            switch (param.type) {
                case ParameterType::INTEGER: {
                    std::uniform_int_distribution<int> int_dist(param.min_int, param.max_int);
                    individual.parameters[param.name] = std::to_string(int_dist(random_generator_));
                    break;
                }

                case ParameterType::FLOAT: {
                    std::uniform_real_distribution<double> float_dist(param.min_float, param.max_float);
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(4) << float_dist(random_generator_);
                    individual.parameters[param.name] = oss.str();
                    break;
                }

                case ParameterType::BOOLEAN: {
                    individual.parameters[param.name] =
                        (individual.parameters[param.name] == "true") ? "false" : "true";
                    break;
                }

                case ParameterType::CATEGORICAL: {
                    if (!param.categories.empty()) {
                        std::uniform_int_distribution<int> cat_dist(0, param.categories.size() - 1);
                        individual.parameters[param.name] = param.categories[cat_dist(random_generator_)];
                    }
                    break;
                }
            }
        }
    }
}

std::map<std::string, std::string> GeneticOptimizer::getRandomParameters() {
    std::map<std::string, std::string> parameters;
    std::uniform_real_distribution<double> float_dist(0.0, 1.0);
    std::uniform_int_distribution<int> int_dist(0, 100);

    for (const auto& param : parameters_) {
        switch (param.type) {
            case ParameterType::INTEGER: {
                int value = param.min_int + int_dist(random_generator_) %
                           (param.max_int - param.min_int + 1);
                parameters[param.name] = std::to_string(value);
                break;
            }

            case ParameterType::FLOAT: {
                double value = param.min_float + float_dist(random_generator_) *
                              (param.max_float - param.min_float);
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(4) << value;
                parameters[param.name] = oss.str();
                break;
            }

            case ParameterType::BOOLEAN: {
                parameters[param.name] = (int_dist(random_generator_) % 2 == 0) ? "true" : "false";
                break;
            }

            case ParameterType::CATEGORICAL: {
                if (!param.categories.empty()) {
                    size_t index = int_dist(random_generator_) % param.categories.size();
                    parameters[param.name] = param.categories[index];
                }
                break;
            }
        }
    }

    return parameters;
}

// GradientDescentOptimizer implementation
GradientDescentOptimizer::GradientDescentOptimizer(double learning_rate, double momentum, size_t batch_size)
    : learning_rate_(learning_rate), momentum_(momentum), batch_size_(batch_size) {}

void GradientDescentOptimizer::initialize(
    const std::vector<OptimizationParameter>& parameters,
    const OptimizationConstraints& constraints,
    OptimizationObjective objective) {

    parameters_ = parameters;
    constraints_ = constraints;
    objective_ = objective;

    // Initialize current parameters to defaults
    for (const auto& param : parameters_) {
        switch (param.type) {
            case ParameterType::INTEGER:
                current_parameters_[param.name] = static_cast<double>(param.default_int);
                break;
            case ParameterType::FLOAT:
                current_parameters_[param.name] = param.default_float;
                break;
            case ParameterType::BOOLEAN:
                current_parameters_[param.name] = param.default_bool ? 1.0 : 0.0;
                break;
            case ParameterType::CATEGORICAL:
                // Find index of default category
                auto it = std::find(param.categories.begin(), param.categories.end(), param.default_category);
                size_t index = std::distance(param.categories.begin(), it);
                current_parameters_[param.name] = static_cast<double>(index);
                break;
        }

        velocities_[param.name] = 0.0;
    }

    first_iteration_ = true;
    gradient_buffer_.clear();

    Logger::info("Initialized Gradient Descent optimizer with learning rate {:.6f}, momentum {:.3f}",
                 learning_rate_, momentum_);
}

std::map<std::string, std::string> GradientDescentOptimizer::getNextParameters() {
    if (first_iteration_) {
        first_iteration_ = false;
        return doubleMapToParameters(current_parameters_);
    }

    if (gradient_buffer_.size() >= batch_size_) {
        // Calculate gradients from batch
        auto gradients = estimateGradients();

        // Update velocities with momentum
        updateVelocities(gradients);

        // Update parameters
        for (auto& [name, value] : current_parameters_) {
            value -= learning_rate_ * velocities_[name];

            // Apply parameter constraints
            const auto& param = *std::find_if(parameters_.begin(), parameters_.end(),
                [&name](const OptimizationParameter& p) { return p.name == name; });

            switch (param.type) {
                case ParameterType::INTEGER:
                    value = std::max(static_cast<double>(param.min_int),
                                   std::min(static_cast<double>(param.max_int), value));
                    break;
                case ParameterType::FLOAT:
                    value = std::max(param.min_float, std::min(param.max_float, value));
                    break;
                case ParameterType::BOOLEAN:
                    value = std::max(0.0, std::min(1.0, value));
                    break;
                case ParameterType::CATEGORICAL:
                    value = std::max(0.0, std::min(static_cast<double>(param.categories.size() - 1), value));
                    break;
            }
        }

        gradient_buffer_.clear();
    }

    return doubleMapToParameters(current_parameters_);
}

void GradientDescentOptimizer::updateResults(
    const std::map<std::string, std::string>& parameters,
    double objective_value,
    const puzzle71::monitoring::PerformanceMetrics& metrics) {

    gradient_buffer_.emplace_back(parameters, objective_value);
    Logger::trace("Gradient descent collected batch sample with objective: {}", objective_value);
}

bool GradientDescentOptimizer::isOptimizationComplete(const OptimizationState& state) const {
    return state.current_iteration >= constraints_.max_iterations ||
           (std::chrono::steady_clock::now() - state.start_time) >= constraints_.max_optimization_time ||
           state.iterations_without_improvement > 30;
}

std::map<std::string, double> GradientDescentOptimizer::getStatistics() const {
    return {
        {"learning_rate", learning_rate_},
        {"momentum", momentum_},
        {"batch_size", static_cast<double>(batch_size_)},
        {"gradient_buffer_size", static_cast<double>(gradient_buffer_.size())}
    };
}

std::map<std::string, double> GradientDescentOptimizer::parametersToDoubleMap(
    const std::map<std::string, std::string>& params) {

    std::map<std::string, double> double_params;

    for (const auto& param : parameters_) {
        auto it = params.find(param.name);
        if (it != params.end()) {
            switch (param.type) {
                case ParameterType::INTEGER:
                    double_params[param.name] = static_cast<double>(std::stoi(it->second));
                    break;
                case ParameterType::FLOAT:
                    double_params[param.name] = std::stod(it->second);
                    break;
                case ParameterType::BOOLEAN:
                    double_params[param.name] = (it->second == "true") ? 1.0 : 0.0;
                    break;
                case ParameterType::CATEGORICAL: {
                    auto cat_it = std::find(param.categories.begin(), param.categories.end(), it->second);
                    size_t index = std::distance(param.categories.begin(), cat_it);
                    double_params[param.name] = static_cast<double>(index);
                    break;
                }
            }
        }
    }

    return double_params;
}

std::map<std::string, std::string> GradientDescentOptimizer::doubleMapToParameters(
    const std::map<std::string, double>& double_params) {

    std::map<std::string, std::string> string_params;

    for (const auto& param : parameters_) {
        auto it = double_params.find(param.name);
        if (it != double_params.end()) {
            switch (param.type) {
                case ParameterType::INTEGER: {
                    int value = static_cast<int>(std::round(it->second));
                    string_params[param.name] = std::to_string(value);
                    break;
                }

                case ParameterType::FLOAT: {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(4) << it->second;
                    string_params[param.name] = oss.str();
                    break;
                }

                case ParameterType::BOOLEAN: {
                    string_params[param.name] = (it->second > 0.5) ? "true" : "false";
                    break;
                }

                case ParameterType::CATEGORICAL: {
                    size_t index = static_cast<size_t>(std::round(it->second));
                    index = std::min(index, param.categories.size() - 1);
                    string_params[param.name] = param.categories[index];
                    break;
                }
            }
        }
    }

    return string_params;
}

void GradientDescentOptimizer::updateVelocities(const std::map<std::string, double>& gradients) {
    for (const auto& [name, gradient] : gradients) {
        velocities_[name] = momentum_ * velocities_[name] + gradient;
    }
}

std::map<std::string, double> GradientDescentOptimizer::estimateGradients() {
    std::map<std::string, double> gradients;

    if (gradient_buffer_.size() < 2) {
        return gradients;
    }

    // Simple finite difference gradient estimation
    // Find the best and worst points in the batch
    auto best_it = std::max_element(gradient_buffer_.begin(), gradient_buffer_.end(),
        [](const auto& a, const auto& b) { return std::get<1>(a) < std::get<1>(b); });

    auto worst_it = std::min_element(gradient_buffer_.begin(), gradient_buffer_.end(),
        [](const auto& a, const auto& b) { return std::get<1>(a) < std::get<1>(b); });

    if (best_it != gradient_buffer_.end() && worst_it != gradient_buffer_.end()) {
        auto best_params = parametersToDoubleMap(std::get<0>(*best_it));
        auto worst_params = parametersToDoubleMap(std::get<0>(*worst_it));

        for (const auto& param : parameters_) {
            double delta = best_params[param.name] - worst_params[param.name];
            double obj_diff = std::get<1>(*best_it) - std::get<1>(*worst_it);

            if (std::abs(delta) > 1e-6) {
                gradients[param.name] = obj_diff / delta;
            } else {
                gradients[param.name] = 0.0;
            }
        }
    }

    return gradients;
}

// AutomatedOptimizer implementation
AutomatedOptimizer::AutomatedOptimizer(int device_id, OptimizationObjective objective)
    : device_id_(device_id), objective_(objective) {

    // Initialize constraints for device
    constraints_ = OptimizerFactory::createDefaultConstraints(device_id);

    // Set default algorithm
    algorithm_ = OptimizerFactory::createAlgorithm("bayesian");

    current_state_.start_time = std::chrono::steady_clock::now();

    Logger::info("Initialized Automated Optimizer for device {} with objective {}",
                 device_id, static_cast<int>(objective));
}

AutomatedOptimizer::~AutomatedOptimizer() {
    stopOptimization();
}

void AutomatedOptimizer::setParameters(const std::vector<OptimizationParameter>& parameters) {
    parameters_ = parameters;

    if (algorithm_) {
        algorithm_->initialize(parameters_, constraints_, objective_);
    }

    Logger::info("Set {} optimization parameters", parameters.size());
}

void AutomatedOptimizer::setConstraints(const OptimizationConstraints& constraints) {
    constraints_ = constraints;
    Logger::info("Updated optimization constraints");
}

void AutomatedOptimizer::setObjective(OptimizationObjective objective) {
    objective_ = objective;
    Logger::info("Updated optimization objective to {}", static_cast<int>(objective));
}

void AutomatedOptimizer::setOptimizationAlgorithm(std::unique_ptr<OptimizationAlgorithm> algorithm) {
    algorithm_ = std::move(algorithm);

    if (algorithm_ && !parameters_.empty()) {
        algorithm_->initialize(parameters_, constraints_, objective_);
    }

    Logger::info("Updated optimization algorithm");
}

OptimizationResult AutomatedOptimizer::optimize(
    std::function<std::map<std::string, std::string>(const std::map<std::string, std::string>&)> evaluation_function,
    std::chrono::seconds timeout) {

    evaluation_function_ = evaluation_function;

    // Initialize optimization state
    current_state_ = OptimizationState{};
    current_state_.start_time = std::chrono::steady_clock::now();
    current_result_ = OptimizationResult{};

    if (!algorithm_) {
        Logger::error("No optimization algorithm configured");
        return current_result_;
    }

    algorithm_->initialize(parameters_, constraints_, objective_);

    // Run optimization loop
    auto start_time = std::chrono::steady_clock::now();

    while (!algorithm_->isOptimizationComplete(current_state_) &&
           (std::chrono::steady_clock::now() - start_time) < timeout) {

        // Get next parameters to evaluate
        auto parameters = algorithm_->getNextParameters();
        if (parameters.empty()) {
            break;
        }

        // Validate parameters
        if (!validateParameters(parameters)) {
            Logger::warn("Invalid parameters generated, skipping");
            continue;
        }

        // Apply constraints
        parameters = constrainParameters(parameters);

        // Evaluate performance
        puzzle71::monitoring::PerformanceMetrics metrics;
        double objective_value = evaluateObjective(parameters, metrics);

        // Update optimization state
        updateState(parameters, objective_value, metrics);

        // Update algorithm
        algorithm_->updateResults(parameters, objective_value, metrics);

        // Check for improvement
        if (objective_value > current_state_.best_objective_value) {
            current_state_.best_objective_value = objective_value;
            current_state_.best_parameters = parameters;
            current_state_.last_improvement_time = std::chrono::steady_clock::now();
            current_state_.iterations_without_improvement = 0;
        } else {
            current_state_.iterations_without_improvement++;
        }

        // Notify progress
        notifyProgress();

        current_state_.current_iteration++;
    }

    // Finalize result
    current_result_.success = !current_state_.best_parameters.empty();
    current_result_.objective_value = current_state_.best_objective_value;
    current_result_.parameter_values = current_state_.best_parameters;
    current_result_.optimization_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    current_result_.iterations_completed = current_state_.current_iteration;
    current_result_.algorithm_used = algorithm_->getAlgorithmName();
    current_result_.objective_history = current_state_.objective_history;

    // Get final performance metrics
    if (current_result_.success) {
        evaluation_function_(current_result_.parameter_values);  // Re-run to get final metrics
        // Note: In production, you'd want to capture the actual metrics from this evaluation
    }

    // Check convergence
    current_state_.is_converged = checkConvergence();

    if (current_state_.is_converged) {
        current_result_.optimization_notes = "Optimization converged successfully";
    } else if (current_state_.current_iteration >= constraints_.max_iterations) {
        current_result_.optimization_notes = "Optimization reached maximum iterations";
    } else {
        current_result_.optimization_notes = "Optimization reached timeout";
    }

    Logger::info("Optimization completed: objective={:.4f}, iterations={}, time={}ms",
                 current_result_.objective_value, current_result_.iterations_completed,
                 current_result_.optimization_time.count());

    // Notify completion
    notifyCompletion();

    return current_result_;
}

OptimizationResult AutomatedOptimizer::optimizeWithMonitor(
    std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor,
    std::chrono::seconds timeout) {

    // Create evaluation function that uses the monitor
    auto evaluation_function = [this, monitor](const std::map<std::string, std::string>& parameters) {
        // Start monitoring if not already running
        if (!monitor->isMonitoring()) {
            monitor->startMonitoring();
        }

        // Apply parameters to the system
        // This would integrate with the actual GPU execution system
        // For now, we simulate the evaluation

        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Simulate execution time

        // Get current metrics from monitor
        auto metrics = monitor->getCurrentMetrics();

        // Update metrics with parameter information
        for (const auto& [name, value] : parameters) {
            if (name == "block_size") {
                metrics.block_size = std::stoul(value);
            } else if (name == "grid_size") {
                metrics.grid_size = std::stoul(value);
            }
        }

        // Store in history
        optimization_history_.emplace_back(parameters, metrics.keys_per_second, metrics);

        return parameters;  // Return the parameters as the "result"
    };

    return optimize(evaluation_function, timeout);
}

void AutomatedOptimizer::startOptimizationAsync(
    std::function<std::map<std::string, std::string>(const std::map<std::string, std::string>&)> evaluation_function) {

    if (optimization_running_.load()) {
        Logger::warn("Optimization is already running");
        return;
    }

    evaluation_function_ = evaluation_function;
    optimization_running_.store(true);

    optimization_thread_ = std::make_unique<std::thread>([this]() {
        current_result_ = optimize(evaluation_function_, constraints_.max_optimization_time);
        optimization_running_.store(false);
    });

    Logger::info("Started asynchronous optimization");
}

bool AutomatedOptimizer::isOptimizationRunning() const {
    return optimization_running_.load();
}

OptimizationResult AutomatedOptimizer::getOptimizationResult() {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_result_;
}

void AutomatedOptimizer::stopOptimization() {
    if (optimization_running_.load()) {
        optimization_running_.store(false);

        if (optimization_thread_ && optimization_thread_->joinable()) {
            optimization_thread_->join();
        }

        Logger::info("Stopped optimization");
    }
}

OptimizationState AutomatedOptimizer::getCurrentState() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_state_;
}

std::map<std::string, std::string> AutomatedOptimizer::getCurrentParameters() const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return current_state_.best_parameters;
}

std::map<std::string, double> AutomatedOptimizer::getOptimizationStatistics() const {
    std::lock_guard<std::mutex> lock(state_mutex_);

    auto stats = algorithm_ ? algorithm_->getStatistics() : std::map<std::string, double>{};

    stats["current_iteration"] = static_cast<double>(current_state_.current_iteration);
    stats["best_objective_value"] = current_state_.best_objective_value;
    stats["iterations_without_improvement"] = static_cast<double>(current_state_.iterations_without_improvement);
    stats["is_converged"] = current_state_.is_converged ? 1.0 : 0.0;

    return stats;
}

std::vector<OptimizationParameter> AutomatedOptimizer::createKernelLaunchParameters() {
    return {
        {"block_size", ParameterType::INTEGER, "CUDA block size", 32, 1024, 256, 0, 0, 0, false, "", 0, 0, false, 256, 1.0},
        {"grid_size", ParameterType::INTEGER, "CUDA grid size", 1, 65535, 256, 0, 0, 0, false, "", 0, 0, false, 256, 1.0},
        {"points_per_thread", ParameterType::INTEGER, "Points processed per thread", 1, 1024, 64, 0, 0, 0, false, "", 0, 0, false, 64, 1.0},
        {"shared_memory_size", ParameterType::INTEGER, "Shared memory size in bytes", 0, 65536, 16384, 0, 0, 0, false, "", 0, 0, false, 16384, 0.5}
    };
}

std::vector<OptimizationParameter> AutomatedOptimizer::createMemoryParameters() {
    return {
        {"memory_pool_size", ParameterType::INTEGER, "Memory pool size in MB", 64, 1024, 256, 0, 0, 0, false, "", 0, 0, false, 256, 1.0},
        {"allocation_strategy", ParameterType::CATEGORICAL, "Memory allocation strategy", 0, 0, 0, 0, 0, false, {"first_fit", "best_fit", "buddy_system"}, "first_fit", 0, 0, false, "", 0.5},
        {"enable_memory_coalescing", ParameterType::BOOLEAN, "Enable memory coalescing optimization", 0, 0, 0, 0, 0, true, "", 0, 0, false, "", 0.8},
        {"prefetch_distance", ParameterType::INTEGER, "Memory prefetch distance", 0, 32, 8, 0, 0, 0, false, "", 0, 0, false, 8, 0.3}
    };
}

std::vector<OptimizationParameter> AutomatedOptimizer::createAlgorithmParameters() {
    return {
        {"kernel_variant", ParameterType::CATEGORICAL, "Kernel implementation variant", 0, 0, 0, 0, 0, false, {"separated", "fused", "memory_optimized"}, "separated", 0, 0, false, "", 1.0},
        {"enable_warp_optimization", ParameterType::BOOLEAN, "Enable warp-level optimizations", 0, 0, 0, 0, 0, true, "", 0, 0, false, "", 0.9},
        {"optimization_level", ParameterType::CATEGORICAL, "Compiler optimization level", 0, 0, 0, 0, 0, false, {"O1", "O2", "O3"}, "O3", 0, 0, false, "", 0.7},
        {"use_deterministic_execution", ParameterType::BOOLEAN, "Use deterministic execution mode", 0, 0, 0, 0, 0, false, "", 0, 0, false, "", 0.2}
    };
}

std::vector<OptimizationParameter> AutomatedOptimizer::createComprehensiveParameters() {
    auto params = createKernelLaunchParameters();
    auto memory_params = createMemoryParameters();
    auto algo_params = createAlgorithmParameters();

    params.insert(params.end(), memory_params.begin(), memory_params.end());
    params.insert(params.end(), algo_params.begin(), algo_params.end());

    return params;
}

std::string AutomatedOptimizer::generateOptimizationReport() const {
    std::lock_guard<std::mutex> lock(state_mutex_);

    std::ostringstream report;
    report << "Automated Optimization Report\n";
    report << "==============================\n\n";

    report << "Device: " << device_id_ << "\n";
    report << "Objective: " << static_cast<int>(objective_) << "\n";
    report << "Algorithm: " << (algorithm_ ? algorithm_->getAlgorithmName() : "None") << "\n\n";

    report << "Optimization State:\n";
    report << "  Current Iteration: " << current_state_.current_iteration << "\n";
    report << "  Best Objective Value: " << current_state_.best_objective_value << "\n";
    report << "  Iterations Without Improvement: " << current_state_.iterations_without_improvement << "\n";
    report << "  Converged: " << (current_state_.is_converged ? "Yes" : "No") << "\n\n";

    if (!current_state_.best_parameters.empty()) {
        report << "Best Parameters:\n";
        for (const auto& [name, value] : current_state_.best_parameters) {
            report << "  " << name << ": " << value << "\n";
        }
        report << "\n";
    }

    report << "Optimization History Size: " << optimization_history_.size() << " entries\n";

    if (current_result_.success) {
        report << "Optimization completed successfully!\n";
        report << "Final Objective: " << current_result_.objective_value << "\n";
        report << "Total Iterations: " << current_result_.iterations_completed << "\n";
        report << "Optimization Time: " << current_result_.optimization_time.count() << "ms\n";
    }

    return report.str();
}

bool AutomatedOptimizer::exportOptimizationHistory(const std::string& filename) const {
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open file for optimization history export: {}", filename);
            return false;
        }

        file << "# Optimization History Export\n";
        file << "# Device: " << device_id_ << "\n";
        file << "# Objective: " << static_cast<int>(objective_) << "\n";
        file << "# Timestamp,Parameters,Objective,KeysPerSecond,GPUUtilization,Temperature\n";

        for (const auto& [params, objective, metrics] : optimization_history_) {
            file << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::system_clock::now().time_since_epoch()).count();

            // Serialize parameters
            file << ",";
            bool first = true;
            for (const auto& [name, value] : params) {
                if (!first) file << ";";
                file << name << "=" << value;
                first = false;
            }

            file << "," << objective;
            file << "," << metrics.keys_per_second;
            file << "," << metrics.gpu_utilization_percent;
            file << "," << metrics.temperature_celsius;
            file << "\n";
        }

        file.close();
        Logger::info("Exported optimization history with {} entries to {}",
                     optimization_history_.size(), filename);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to export optimization history: {}", e.what());
        return false;
    }
}

bool AutomatedOptimizer::loadOptimizationHistory(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            Logger::error("Failed to open optimization history file: {}", filename);
            return false;
        }

        optimization_history_.clear();

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') {
                continue;
            }

            std::istringstream iss(line);
            std::string token;
            std::vector<std::string> tokens;

            while (std::getline(iss, token, ',')) {
                tokens.push_back(token);
            }

            if (tokens.size() >= 5) {
                // Parse parameters (token[1])
                std::map<std::string, std::string> params;
                std::istringstream param_stream(tokens[1]);
                std::string param_pair;

                while (std::getline(param_stream, param_pair, ';')) {
                    size_t eq_pos = param_pair.find('=');
                    if (eq_pos != std::string::npos) {
                        std::string name = param_pair.substr(0, eq_pos);
                        std::string value = param_pair.substr(eq_pos + 1);
                        params[name] = value;
                    }
                }

                // Parse other fields
                double objective = std::stod(tokens[2]);

                puzzle71::monitoring::PerformanceMetrics metrics;
                metrics.keys_per_second = std::stod(tokens[3]);
                metrics.gpu_utilization_percent = std::stod(tokens[4]);
                if (tokens.size() > 5) {
                    metrics.temperature_celsius = std::stod(tokens[5]);
                }

                optimization_history_.emplace_back(params, objective, metrics);
            }
        }

        file.close();
        Logger::info("Loaded optimization history with {} entries from {}",
                     optimization_history_.size(), filename);
        return true;

    } catch (const std::exception& e) {
        Logger::error("Failed to load optimization history: {}", e.what());
        return false;
    }
}

void AutomatedOptimizer::setProgressCallback(ProgressCallback callback) {
    progress_callback_ = callback;
}

void AutomatedOptimizer::setCompletionCallback(CompletionCallback callback) {
    completion_callback_ = callback;
}

void AutomatedOptimizer::optimizationLoop(std::chrono::seconds timeout) {
    current_result_ = optimize(evaluation_function_, timeout);
}

double AutomatedOptimizer::evaluateObjective(
    const std::map<std::string, std::string>& parameters,
    puzzle71::monitoring::PerformanceMetrics& metrics) {

    // Execute evaluation function
    auto result = evaluation_function_(parameters);

    // For this implementation, we simulate the objective based on parameters
    // In production, this would be actual GPU performance measurement

    double base_throughput = 1000000.0;  // 1M keys/s baseline

    // Apply parameter effects (simulated)
    for (const auto& [name, value] : parameters) {
        if (name == "block_size") {
            int block_size = std::stoi(value);
            // Optimal around 256-512
            double efficiency = 1.0 - std::abs(block_size - 384.0) / 384.0;
            base_throughput *= (0.5 + 0.5 * efficiency);
        } else if (name == "points_per_thread") {
            int ppt = std::stoi(value);
            // Optimal around 64-128
            double efficiency = 1.0 - std::abs(ppt - 96.0) / 96.0;
            base_throughput *= (0.7 + 0.3 * efficiency);
        } else if (name == "enable_warp_optimization") {
            if (value == "true") {
                base_throughput *= 1.2;  // 20% improvement
            }
        }
    }

    // Add some noise
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<double> noise(0.0, base_throughput * 0.05);
    base_throughput += noise(gen);

    // Create simulated metrics
    metrics.keys_per_second = base_throughput;
    metrics.gpu_utilization_percent = 75.0 + (base_throughput / 1000000.0) * 10.0;
    metrics.temperature_celsius = 65.0 + (base_throughput / 1000000.0) * 5.0;
    metrics.memory_utilization_percent = 60.0 + (base_throughput / 1000000.0) * 15.0;

    // Calculate objective based on optimization objective
    switch (objective_) {
        case OptimizationObjective::MAXIMIZE_THROUGHPUT:
            return base_throughput;

        case OptimizationObjective::MINIMIZE_LATENCY:
            return 1.0 / (1000.0 / base_throughput);  // Inverse of latency per key

        case OptimizationObjective::MINIMIZE_POWER_USAGE:
            return 1.0 / (200.0 + base_throughput / 10000.0);  // Lower power is better

        case OptimizationObjective::MAXIMIZE_EFFICIENCY:
            return (base_throughput / 1000000.0) * (metrics.gpu_utilization_percent / 100.0);

        case OptimizationObjective::BALANCED_PERFORMANCE:
            return base_throughput * 0.7 + (metrics.gpu_utilization_percent / 100.0) * base_throughput * 0.3;

        default:
            return base_throughput;
    }
}

void AutomatedOptimizer::updateState(
    const std::map<std::string, std::string>& parameters,
    double objective_value,
    const puzzle71::monitoring::PerformanceMetrics& metrics) {

    std::lock_guard<std::mutex> lock(state_mutex_);

    current_state_.current_iteration++;
    current_state_.objective_history.push_back(objective_value);

    // Store in history
    optimization_history_.emplace_back(parameters, objective_value, metrics);
}

bool AutomatedOptimizer::checkConvergence() const {
    if (current_state_.objective_history.size() < 10) {
        return false;
    }

    // Check if recent improvements are below threshold
    size_t window_size = std::min(size_t(10), current_state_.objective_history.size());
    auto recent_begin = current_state_.objective_history.end() - window_size;

    double recent_avg = std::accumulate(recent_begin, current_state_.objective_history.end(), 0.0) / window_size;
    double overall_best = *std::max_element(current_state_.objective_history.begin(),
                                          current_state_.objective_history.end());

    return (overall_best - recent_avg) < current_state_.convergence_threshold * std::abs(overall_best);
}

void AutomatedOptimizer::notifyProgress() {
    if (progress_callback_) {
        progress_callback_(current_state_);
    }
}

void AutomatedOptimizer::notifyCompletion() {
    if (completion_callback_) {
        completion_callback_(current_result_);
    }
}

bool AutomatedOptimizer::validateParameters(const std::map<std::string, std::string>& parameters) const {
    // Check if all required parameters are present
    for (const auto& param : parameters_) {
        if (parameters.find(param.name) == parameters.end()) {
            return false;
        }
    }

    // Check parameter ranges
    for (const auto& param : parameters_) {
        auto it = parameters.find(param.name);
        if (it != parameters.end()) {
            switch (param.type) {
                case ParameterType::INTEGER: {
                    try {
                        int value = std::stoi(it->second);
                        if (value < param.min_int || value > param.max_int) {
                            return false;
                        }
                    } catch (...) {
                        return false;
                    }
                    break;
                }

                case ParameterType::FLOAT: {
                    try {
                        double value = std::stod(it->second);
                        if (value < param.min_float || value > param.max_float) {
                            return false;
                        }
                    } catch (...) {
                        return false;
                    }
                    break;
                }

                case ParameterType::BOOLEAN: {
                    if (it->second != "true" && it->second != "false") {
                        return false;
                    }
                    break;
                }

                case ParameterType::CATEGORICAL: {
                    if (std::find(param.categories.begin(), param.categories.end(), it->second) == param.categories.end()) {
                        return false;
                    }
                    break;
                }
            }
        }
    }

    return true;
}

std::map<std::string, std::string> AutomatedOptimizer::constrainParameters(const std::map<std::string, std::string>& parameters) const {
    // Apply additional constraints beyond parameter ranges
    auto constrained = parameters;

    // Example: Ensure grid_size * block_size doesn't exceed device limits
    auto grid_it = constrained.find("grid_size");
    auto block_it = constrained.find("block_size");

    if (grid_it != constrained.end() && block_it != constrained.end()) {
        try {
            uint32_t grid_size = std::stoul(grid_it->second);
            uint32_t block_size = std::stoul(block_it->second);

            if (grid_size * block_size > constraints_.max_grid_size) {
                // Scale down proportionally
                double scale = static_cast<double>(constraints_.max_grid_size) / (grid_size * block_size);
                grid_size = static_cast<uint32_t>(grid_size * scale);
                block_size = static_cast<uint32_t>(block_size * scale);

                // Ensure minimum values
                grid_size = std::max(grid_size, 1u);
                block_size = std::max(block_size, 32u);

                constrained["grid_size"] = std::to_string(grid_size);
                constrained["block_size"] = std::to_string(block_size);

                Logger::debug("Scaled grid_size to {} and block_size to {} to satisfy constraints",
                             grid_size, block_size);
            }
        } catch (...) {
            // If parsing fails, keep original values
        }
    }

    return constrained;
}

// OptimizerFactory implementation
std::unique_ptr<AutomatedOptimizer> OptimizerFactory::create(
    int device_id, OptimizationObjective objective) {

    return std::make_unique<AutomatedOptimizer>(device_id, objective);
}

std::unique_ptr<AutomatedOptimizer> OptimizerFactory::createKernelLaunchOptimizer(int device_id) {
    auto optimizer = std::make_unique<AutomatedOptimizer>(device_id, OptimizationObjective::MAXIMIZE_THROUGHPUT);
    optimizer->setParameters(AutomatedOptimizer::createKernelLaunchParameters());
    optimizer->setOptimizationAlgorithm(createAlgorithm("bayesian"));
    return optimizer;
}

std::unique_ptr<AutomatedOptimizer> OptimizerFactory::createMemoryOptimizer(int device_id) {
    auto optimizer = std::make_unique<AutomatedOptimizer>(device_id, OptimizationObjective::MAXIMIZE_EFFICIENCY);
    optimizer->setParameters(AutomatedOptimizer::createMemoryParameters());
    optimizer->setOptimizationAlgorithm(createAlgorithm("genetic"));
    return optimizer;
}

std::unique_ptr<AutomatedOptimizer> OptimizerFactory::createComprehensiveOptimizer(int device_id) {
    auto optimizer = std::make_unique<AutomatedOptimizer>(device_id, OptimizationObjective::BALANCED_PERFORMANCE);
    optimizer->setParameters(AutomatedOptimizer::createComprehensiveParameters());
    optimizer->setConstraints(createHighPerformanceConstraints(device_id));
    optimizer->setOptimizationAlgorithm(createAlgorithm("bayesian"));
    return optimizer;
}

std::unique_ptr<OptimizationAlgorithm> OptimizerFactory::createAlgorithm(
    const std::string& algorithm_name, const std::map<std::string, double>& parameters) {

    if (algorithm_name == "grid_search") {
        size_t samples = static_cast<size_t>(parameters.count("samples") ? parameters.at("samples") : 10);
        return std::make_unique<GridSearchOptimizer>(samples);
    } else if (algorithm_name == "bayesian") {
        size_t initial_samples = static_cast<size_t>(parameters.count("initial_samples") ? parameters.at("initial_samples") : 10);
        return std::make_unique<BayesianOptimizer>(initial_samples);
    } else if (algorithm_name == "genetic") {
        size_t population_size = static_cast<size_t>(parameters.count("population_size") ? parameters.at("population_size") : 50);
        double mutation_rate = parameters.count("mutation_rate") ? parameters.at("mutation_rate") : 0.1;
        double crossover_rate = parameters.count("crossover_rate") ? parameters.at("crossover_rate") : 0.8;
        return std::make_unique<GeneticOptimizer>(population_size, mutation_rate, crossover_rate);
    } else if (algorithm_name == "gradient_descent") {
        double learning_rate = parameters.count("learning_rate") ? parameters.at("learning_rate") : 0.01;
        double momentum = parameters.count("momentum") ? parameters.at("momentum") : 0.9;
        size_t batch_size = static_cast<size_t>(parameters.count("batch_size") ? parameters.at("batch_size") : 1);
        return std::make_unique<GradientDescentOptimizer>(learning_rate, momentum, batch_size);
    }

    Logger::error("Unknown optimization algorithm: {}", algorithm_name);
    return nullptr;
}

OptimizationConstraints OptimizerFactory::createDefaultConstraints(int device_id) {
    OptimizationConstraints constraints;

    // Get device properties for hardware constraints
    cudaDeviceProp props;
    if (cudaGetDeviceProperties(&props, device_id) == cudaSuccess) {
        constraints.max_grid_size = props.maxGridSize[0];
        constraints.max_block_size = props.maxThreadsPerBlock;
        constraints.max_registers_per_thread = props.regsPerBlock;
        constraints.max_shared_memory_bytes = props.sharedMemPerBlock;
        constraints.max_memory_mb = props.totalGlobalMem / (1024 * 1024);
    }

    // Performance constraints
    constraints.min_throughput = 500000.0;  // 500K keys/s minimum
    constraints.max_latency_ms = 1000.0;    // 1 second maximum
    constraints.max_power_watts = 350.0;    // 350W maximum

    // Time constraints
    constraints.max_optimization_time = std::chrono::seconds(300);  // 5 minutes
    constraints.max_iterations = 1000;

    return constraints;
}

OptimizationConstraints OptimizerFactory::createHighPerformanceConstraints(int device_id) {
    auto constraints = createDefaultConstraints(device_id);

    // Stricter performance requirements
    constraints.min_throughput = 2000000.0;  // 2M keys/s minimum
    constraints.max_latency_ms = 500.0;      // 500ms maximum

    // Longer optimization time for better results
    constraints.max_optimization_time = std::chrono::seconds(600);  // 10 minutes
    constraints.max_iterations = 2000;

    return constraints;
}

OptimizationConstraints OptimizerFactory::createPowerEfficientConstraints(int device_id) {
    auto constraints = createDefaultConstraints(device_id);

    // Power constraints
    constraints.max_power_watts = 250.0;    // 250W limit for power efficiency

    // Relaxed performance constraints
    constraints.min_throughput = 1000000.0;  // 1M keys/s minimum
    constraints.max_latency_ms = 2000.0;     // 2 second maximum

    // Shorter optimization time
    constraints.max_optimization_time = std::chrono::seconds(180);  // 3 minutes
    constraints.max_iterations = 500;

    return constraints;
}

// Optimization utilities implementation
namespace optimization_utils {

SensitivityAnalysis analyzeSensitivity(
    const std::vector<std::tuple<std::map<std::string, std::string>, double>>& evaluations,
    const std::vector<OptimizationParameter>& parameters) {

    SensitivityAnalysis analysis;

    if (evaluations.empty() || parameters.empty()) {
        return analysis;
    }

    // Calculate sensitivity for each parameter
    for (const auto& param : parameters) {
        std::vector<double> values;
        std::vector<double> objectives;

        // Extract values and objectives for this parameter
        for (const auto& [params, objective] : evaluations) {
            auto it = params.find(param.name);
            if (it != params.end()) {
                double value = 0.0;
                switch (param.type) {
                    case ParameterType::INTEGER:
                        value = static_cast<double>(std::stoi(it->second));
                        break;
                    case ParameterType::FLOAT:
                        value = std::stod(it->second);
                        break;
                    case ParameterType::BOOLEAN:
                        value = (it->second == "true") ? 1.0 : 0.0;
                        break;
                    case ParameterType::CATEGORICAL: {
                        auto cat_it = std::find(param.categories.begin(), param.categories.end(), it->second);
                        size_t index = std::distance(param.categories.begin(), cat_it);
                        value = static_cast<double>(index);
                        break;
                    }
                }
                values.push_back(value);
                objectives.push_back(objective);
            }
        }

        if (values.size() >= 3) {
            // Calculate correlation coefficient
            double mean_x = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
            double mean_y = std::accumulate(objectives.begin(), objectives.end(), 0.0) / objectives.size();

            double numerator = 0.0;
            double sum_sq_x = 0.0;
            double sum_sq_y = 0.0;

            for (size_t i = 0; i < values.size(); ++i) {
                double dx = values[i] - mean_x;
                double dy = objectives[i] - mean_y;
                numerator += dx * dy;
                sum_sq_x += dx * dx;
                sum_sq_y += dy * dy;
            }

            double correlation = 0.0;
            if (sum_sq_x > 0 && sum_sq_y > 0) {
                correlation = std::abs(numerator / std::sqrt(sum_sq_x * sum_sq_y));
            }

            analysis.sensitivity_scores[param.name] = correlation;
            analysis.parameter_importance[param.name] = correlation * param.importance_weight;
        }
    }

    // Sort parameters by sensitivity
    std::vector<std::pair<std::string, double>> sorted_params(
        analysis.sensitivity_scores.begin(), analysis.sensitivity_scores.end());

    std::sort(sorted_params.begin(), sorted_params.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Categorize parameters
    size_t total_params = sorted_params.size();
    for (size_t i = 0; i < total_params; ++i) {
        if (i < total_params / 3) {
            analysis.most_sensitive_parameters.push_back(sorted_params[i].first);
        } else if (i >= 2 * total_params / 3) {
            analysis.least_sensitive_parameters.push_back(sorted_params[i].first);
        }
    }

    return analysis;
}

std::vector<OptimizationRecommendation> generateRecommendations(
    const OptimizationResult& result,
    const std::vector<OptimizationParameter>& parameters,
    const puzzle71::monitoring::PerformanceMetrics& current_metrics) {

    std::vector<OptimizationRecommendation> recommendations;

    // Generate recommendations based on performance metrics
    if (current_metrics.gpu_utilization_percent < 70.0) {
        OptimizationRecommendation rec;
        rec.parameter_name = "block_size";
        rec.current_value = "256";
        rec.recommended_value = "512";
        rec.expected_improvement = 15.0;
        rec.reasoning = "Low GPU utilization suggests increasing block size may improve occupancy";
        rec.confidence = 0.8;
        recommendations.push_back(rec);
    }

    if (current_metrics.memory_utilization_percent > 85.0) {
        OptimizationRecommendation rec;
        rec.parameter_name = "points_per_thread";
        rec.current_value = "64";
        rec.recommended_value = "32";
        rec.expected_improvement = 10.0;
        rec.reasoning = "High memory usage suggests reducing points per thread may reduce memory pressure";
        rec.confidence = 0.7;
        recommendations.push_back(rec);
    }

    if (current_metrics.temperature_celsius > 80.0) {
        OptimizationRecommendation rec;
        rec.parameter_name = "grid_size";
        rec.current_value = "256";
        rec.recommended_value = "128";
        rec.expected_improvement = 5.0;
        rec.reasoning = "High temperature suggests reducing concurrent kernels may lower thermal load";
        rec.confidence = 0.6;
        recommendations.push_back(rec);
    }

    return recommendations;
}

AlgorithmComparison compareAlgorithms(
    int device_id,
    const std::vector<OptimizationParameter>& parameters,
    const OptimizationConstraints& constraints,
    OptimizationObjective objective,
    const std::vector<std::string>& algorithm_names,
    std::function<std::map<std::string, std::string>(const std::map<std::string, std::string>&)> evaluation_function) {

    AlgorithmComparison comparison;

    for (const auto& algorithm_name : algorithm_names) {
        auto optimizer = OptimizerFactory::create(device_id, objective);
        optimizer->setParameters(parameters);
        optimizer->setConstraints(constraints);
        optimizer->setOptimizationAlgorithm(OptimizerFactory::createAlgorithm(algorithm_name));

        auto start_time = std::chrono::high_resolution_clock::now();
        auto result = optimizer->optimize(evaluation_function, constraints.max_optimization_time);
        auto end_time = std::chrono::high_resolution_clock::now();

        auto optimization_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        result.optimization_time = optimization_time;
        result.algorithm_used = algorithm_name;

        comparison.results[algorithm_name] = result;

        if (result.objective_value > comparison.best_objective_value) {
            comparison.best_objective_value = result.objective_value;
            comparison.best_algorithm = algorithm_name;
        }

        if (optimization_time < comparison.fastest_time) {
            comparison.fastest_time = optimization_time;
        }

        Logger::info("Algorithm comparison: {} achieved objective {:.4f} in {}ms",
                     algorithm_name, result.objective_value, optimization_time.count());
    }

    return comparison;
}

} // namespace optimization_utils

} // namespace puzzle71::optimization