// Puzzle71Solver - Automated Performance Optimization Engine (T046)
// Phase 6: User Story 4 - Performance Monitoring
// AI-driven parameter tuning and automated performance optimization

#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>

#include "core/uint256.h"
#include "monitoring/real_time_monitor.h"
#include "compute/gpu/launch_config.h"
#include "compute/gpu/batch_planner.h"

namespace puzzle71::optimization {

/**
 * @brief Optimization parameter types
 */
enum class ParameterType {
    INTEGER,        // Integer parameters (e.g., block size, grid size)
    FLOAT,          // Floating-point parameters (e.g., thresholds, ratios)
    BOOLEAN,        // Boolean parameters (e.g., enable/disable features)
    CATEGORICAL     // Categorical parameters (e.g., kernel variants)
};

/**
 * @brief Optimization parameter definition
 */
struct OptimizationParameter {
    std::string name;
    ParameterType type;
    std::string description;

    // Parameter ranges
    int min_int{0}, max_int{1}, default_int{0};
    double min_float{0.0}, max_float{1.0}, default_float{0.0};
    bool default_bool{false};
    std::vector<std::string> categories;
    std::string default_category;

    // Current value
    int current_int{0};
    double current_float{0.0};
    bool current_bool{false};
    std::string current_category;

    // Optimization hints
    double importance_weight{1.0};  // Relative importance for optimization
    bool is_discrete{true};         // Whether parameter changes are discrete
    std::function<double(int, double, bool, std::string)> cost_function;  // Optional custom cost function
};

/**
 * @brief Optimization objective types
 */
enum class OptimizationObjective {
    MAXIMIZE_THROUGHPUT,     // Maximize keys per second
    MINIMIZE_LATENCY,        // Minimize kernel execution time
    MINIMIZE_POWER_USAGE,    // Minimize power consumption
    MAXIMIZE_EFFICIENCY,     // Maximize compute efficiency
    BALANCED_PERFORMANCE,    // Balance multiple objectives
    CUSTOM                   // Custom objective function
};

/**
 * @brief Optimization result
 */
struct OptimizationResult {
    bool success{false};
    double objective_value{0.0};
    std::map<std::string, std::string> parameter_values;
    std::chrono::milliseconds optimization_time{0};
    size_t iterations_completed{0};
    std::vector<double> objective_history;
    std::string algorithm_used;
    std::string optimization_notes;

    // Performance metrics at optimal point
    puzzle71::monitoring::PerformanceMetrics performance_metrics;

    /**
     * @brief Calculate improvement over baseline
     */
    double calculateImprovement(const OptimizationResult& baseline) const;

    /**
     * @brief Serialize result to JSON
     */
    std::string toJson() const;

    /**
     * @brief Deserialize result from JSON
     */
    static OptimizationResult fromJson(const std::string& json);
};

/**
 * @brief Optimization constraints
 */
struct OptimizationConstraints {
    // Performance constraints
    double min_throughput{0.0};           // Minimum throughput (keys/s)
    double max_latency_ms{std::numeric_limits<double>::max()};  // Maximum latency
    double max_power_watts{std::numeric_limits<double>::max()}; // Maximum power
    double max_memory_mb{std::numeric_limits<double>::max()};    // Maximum memory

    // Hardware constraints
    std::uint32_t max_grid_size{std::numeric_limits<std::uint32_t>::max()};
    std::uint32_t max_block_size{1024};
    std::uint32_t max_registers_per_thread{80};
    std::uint64_t max_shared_memory_bytes{std::numeric_limits<std::uint64_t>::max()};

    // Time constraints
    std::chrono::seconds max_optimization_time{std::chrono::seconds(300)};  // 5 minutes
    size_t max_iterations{1000};

    /**
     * @brief Check if configuration satisfies constraints
     */
    bool satisfiesConstraints(const puzzle71::monitoring::PerformanceMetrics& metrics,
                            const std::map<std::string, std::string>& parameters) const;
};

/**
 * @brief Optimization state for tracking progress
 */
struct OptimizationState {
    size_t current_iteration{0};
    double best_objective_value{std::numeric_limits<double>::lowest()};
    std::map<std::string, std::string> best_parameters;
    std::vector<double> objective_history;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_improvement_time;
    size_t iterations_without_improvement{0};
    bool is_converged{false};
    double convergence_threshold{0.001};  // 0.1% improvement threshold
};

/**
 * @brief Base optimization algorithm interface
 */
class OptimizationAlgorithm {
public:
    virtual ~OptimizationAlgorithm() = default;

    /**
     * @brief Initialize optimization algorithm
     */
    virtual void initialize(
        const std::vector<OptimizationParameter>& parameters,
        const OptimizationConstraints& constraints,
        OptimizationObjective objective
    ) = 0;

    /**
     * @brief Get next set of parameters to evaluate
     */
    virtual std::map<std::string, std::string> getNextParameters() = 0;

    /**
     * @brief Update algorithm with evaluation results
     */
    virtual void updateResults(
        const std::map<std::string, std::string>& parameters,
        double objective_value,
        const puzzle71::monitoring::PerformanceMetrics& metrics
    ) = 0;

    /**
     * @brief Check if optimization is complete
     */
    virtual bool isOptimizationComplete(const OptimizationState& state) const = 0;

    /**
     * @brief Get algorithm statistics
     */
    virtual std::map<std::string, double> getStatistics() const = 0;

    /**
     * @brief Get algorithm name
     */
    virtual std::string getAlgorithmName() const = 0;
};

/**
 * @brief Grid search optimization algorithm
 */
class GridSearchOptimizer : public OptimizationAlgorithm {
public:
    explicit GridSearchOptimizer(size_t samples_per_dimension = 10);

    void initialize(
        const std::vector<OptimizationParameter>& parameters,
        const OptimizationConstraints& constraints,
        OptimizationObjective objective
    ) override;

    std::map<std::string, std::string> getNextParameters() override;

    void updateResults(
        const std::map<std::string, std::string>& parameters,
        double objective_value,
        const puzzle71::monitoring::PerformanceMetrics& metrics
    ) override;

    bool isOptimizationComplete(const OptimizationState& state) const override;

    std::map<std::string, double> getStatistics() const override;

    std::string getAlgorithmName() const override { return "Grid Search"; }

private:
    size_t samples_per_dimension_;
    std::vector<OptimizationParameter> parameters_;
    OptimizationConstraints constraints_;
    OptimizationObjective objective_;

    std::vector<std::vector<std::string>> parameter_values_;
    std::vector<size_t> current_indices_;
    bool enumeration_complete_{false};

    void generateParameterGrid();
    std::map<std::string, std::string> getCurrentParameters();
    void advanceToNextCombination();
};

/**
 * @brief Bayesian optimization algorithm
 */
class BayesianOptimizer : public OptimizationAlgorithm {
public:
    explicit BayesianOptimizer(size_t initial_samples = 10);

    void initialize(
        const std::vector<OptimizationParameter>& parameters,
        const OptimizationConstraints& constraints,
        OptimizationObjective objective
    ) override;

    std::map<std::string, std::string> getNextParameters() override;

    void updateResults(
        const std::map<std::string, std::string>& parameters,
        double objective_value,
        const puzzle71::monitoring::PerformanceMetrics& metrics
    ) override;

    bool isOptimizationComplete(const OptimizationState& state) const override;

    std::map<std::string, double> getStatistics() const override;

    std::string getAlgorithmName() const override { return "Bayesian Optimization"; }

private:
    size_t initial_samples_;
    std::vector<OptimizationParameter> parameters_;
    OptimizationConstraints constraints_;
    OptimizationObjective objective_;

    struct SamplePoint {
        std::map<std::string, std::string> parameters;
        double objective_value;
        puzzle71::monitoring::PerformanceMetrics metrics;
    };

    std::vector<SamplePoint> samples_;
    bool initial_sampling_complete_{false};
    std::mt19937 random_generator_;

    void generateInitialSample(std::map<std::string, std::string>& parameters);
    std::map<std::string, std::string> generateCandidateFromSurrogate();
    double evaluateAcquisitionFunction(const std::map<std::string, std::string>& parameters);
    std::vector<double> parametersToVector(const std::map<std::string, std::string>& parameters);
    std::map<std::string, std::string> vectorToParameters(const std::vector<double>& vector);
};

/**
 * @brief Genetic algorithm optimizer
 */
class GeneticOptimizer : public OptimizationAlgorithm {
public:
    explicit GeneticOptimizer(
        size_t population_size = 50,
        double mutation_rate = 0.1,
        double crossover_rate = 0.8
    );

    void initialize(
        const std::vector<OptimizationParameter>& parameters,
        const OptimizationConstraints& constraints,
        OptimizationObjective objective
    ) override;

    std::map<std::string, std::string> getNextParameters() override;

    void updateResults(
        const std::map<std::string, std::string>& parameters,
        double objective_value,
        const puzzle71::monitoring::PerformanceMetrics& metrics
    ) override;

    bool isOptimizationComplete(const OptimizationState& state) const override;

    std::map<std::string, double> getStatistics() const override;

    std::string getAlgorithmName() const override { return "Genetic Algorithm"; }

private:
    size_t population_size_;
    double mutation_rate_;
    double crossover_rate_;

    struct Individual {
        std::map<std::string, std::string> parameters;
        double fitness{0.0};
        puzzle71::monitoring::PerformanceMetrics metrics;
    };

    std::vector<OptimizationParameter> parameters_;
    OptimizationConstraints constraints_;
    OptimizationObjective objective_;

    std::vector<Individual> population_;
    std::vector<Individual> offspring_;
    size_t current_individual_{0};
    size_t generation_{0};
    std::mt19937 random_generator_;

    void initializePopulation();
    void evaluatePopulation();
    void selection();
    void crossover();
    void mutation();
    Individual createRandomIndividual();
    Individual crossoverIndividuals(const Individual& parent1, const Individual& parent2);
    void mutateIndividual(Individual& individual);
    std::map<std::string, std::string> getRandomParameters();
};

/**
 * @brief Gradient descent optimizer
 */
class GradientDescentOptimizer : public OptimizationAlgorithm {
public:
    explicit GradientDescentOptimizer(
        double learning_rate = 0.01,
        double momentum = 0.9,
        size_t batch_size = 1
    );

    void initialize(
        const std::vector<OptimizationParameter>& parameters,
        const OptimizationConstraints& constraints,
        OptimizationObjective objective
    ) override;

    std::map<std::string, std::string> getNextParameters() override;

    void updateResults(
        const std::map<std::string, std::string>& parameters,
        double objective_value,
        const puzzle71::monitoring::PerformanceMetrics& metrics
    ) override;

    bool isOptimizationComplete(const OptimizationState& state) const override;

    std::map<std::string, double> getStatistics() const override;

    std::string getAlgorithmName() const override { return "Gradient Descent"; }

private:
    double learning_rate_;
    double momentum_;
    size_t batch_size_;

    std::vector<OptimizationParameter> parameters_;
    OptimizationConstraints constraints_;
    OptimizationObjective objective_;

    std::map<std::string, double> current_parameters_;
    std::map<std::string, double> velocities_;
    std::vector<std::tuple<std::map<std::string, std::string>, double>> gradient_buffer_;
    bool first_iteration_{true};

    std::map<std::string, double> parametersToDoubleMap(const std::map<std::string, std::string>& params);
    std::map<std::string, std::string> doubleMapToParameters(const std::map<std::string, double>& params);
    void updateVelocities(const std::map<std::string, double>& gradients);
    std::map<std::string, double> estimateGradients();
};

/**
 * @brief Automated performance optimizer
 *
 * Main class that coordinates optimization algorithms, parameter management,
 * and performance evaluation for automated GPU kernel optimization.
 */
class AutomatedOptimizer {
public:
    explicit AutomatedOptimizer(
        int device_id,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT
    );

    ~AutomatedOptimizer();

    // Configuration
    void setParameters(const std::vector<OptimizationParameter>& parameters);
    void setConstraints(const OptimizationConstraints& constraints);
    void setObjective(OptimizationObjective objective);
    void setOptimizationAlgorithm(std::unique_ptr<OptimizationAlgorithm> algorithm);

    // Optimization execution
    OptimizationResult optimize(
        std::function<std::map<std::string, std::string>(const std::map<std::string, std::string>&)> evaluation_function,
        std::chrono::seconds timeout = std::chrono::seconds(300)
    );

    OptimizationResult optimizeWithMonitor(
        std::shared_ptr<puzzle71::monitoring::RealTimeMonitor> monitor,
        std::chrono::seconds timeout = std::chrono::seconds(300)
    );

    // Asynchronous optimization
    void startOptimizationAsync(
        std::function<std::map<std::string, std::string>(const std::map<std::string, std::string>&)> evaluation_function
    );

    bool isOptimizationRunning() const;
    OptimizationResult getOptimizationResult();
    void stopOptimization();

    // Current state
    OptimizationState getCurrentState() const;
    std::map<std::string, std::string> getCurrentParameters() const;
    std::map<std::string, double> getOptimizationStatistics() const;

    // Built-in parameter sets
    static std::vector<OptimizationParameter> createKernelLaunchParameters();
    static std::vector<OptimizationParameter> createMemoryParameters();
    static std::vector<OptimizationParameter> createAlgorithmParameters();
    static std::vector<OptimizationParameter> createComprehensiveParameters();

    // Utility methods
    std::string generateOptimizationReport() const;
    bool exportOptimizationHistory(const std::string& filename) const;
    bool loadOptimizationHistory(const std::string& filename);

    // Callback registration
    using ProgressCallback = std::function<void(const OptimizationState&)>;
    void setProgressCallback(ProgressCallback callback);

    using CompletionCallback = std::function<void(const OptimizationResult&)>;
    void setCompletionCallback(CompletionCallback callback);

private:
    int device_id_;
    OptimizationObjective objective_;
    std::vector<OptimizationParameter> parameters_;
    OptimizationConstraints constraints_;
    std::unique_ptr<OptimizationAlgorithm> algorithm_;

    // Asynchronous execution
    std::atomic<bool> optimization_running_{false};
    std::unique_ptr<std::thread> optimization_thread_;
    OptimizationResult current_result_;
    OptimizationState current_state_;
    mutable std::mutex state_mutex_;

    // Callbacks
    ProgressCallback progress_callback_;
    CompletionCallback completion_callback_;

    // Evaluation function
    std::function<std::map<std::string, std::string>(const std::map<std::string, std::string>&)> evaluation_function_;

    // History
    std::vector<std::tuple<std::map<std::string, std::string>, double, puzzle71::monitoring::PerformanceMetrics>> optimization_history_;

    // Private methods
    void optimizationLoop(std::chrono::seconds timeout);
    double evaluateObjective(
        const std::map<std::string, std::string>& parameters,
        puzzle71::monitoring::PerformanceMetrics& metrics
    );

    void updateState(
        const std::map<std::string, std::string>& parameters,
        double objective_value,
        const puzzle71::monitoring::PerformanceMetrics& metrics
    );

    bool checkConvergence() const;
    void notifyProgress();
    void notifyCompletion();

    // Parameter validation and conversion
    bool validateParameters(const std::map<std::string, std::string>& parameters) const;
    std::map<std::string, std::string> constrainParameters(const std::map<std::string, std::string>& parameters) const;
};

/**
 * @brief Factory for creating optimizers and configurations
 */
class OptimizerFactory {
public:
    /**
     * @brief Create optimizer with default parameters
     */
    static std::unique_ptr<AutomatedOptimizer> create(
        int device_id,
        OptimizationObjective objective = OptimizationObjective::MAXIMIZE_THROUGHPUT
    );

    /**
     * @brief Create optimizer for kernel launch optimization
     */
    static std::unique_ptr<AutomatedOptimizer> createKernelLaunchOptimizer(int device_id);

    /**
     * @brief Create optimizer for memory optimization
     */
    static std::unique_ptr<AutomatedOptimizer> createMemoryOptimizer(int device_id);

    /**
     * @brief Create optimizer for comprehensive optimization
     */
    static std::unique_ptr<AutomatedOptimizer> createComprehensiveOptimizer(int device_id);

    /**
     * @brief Create optimization algorithm by name
     */
    static std::unique_ptr<OptimizationAlgorithm> createAlgorithm(
        const std::string& algorithm_name,
        const std::map<std::string, double>& parameters = {}
    );

    /**
     * @brief Create default constraints for device
     */
    static OptimizationConstraints createDefaultConstraints(int device_id);

    /**
     * @brief Create high-performance constraints
     */
    static OptimizationConstraints createHighPerformanceConstraints(int device_id);

    /**
     * @brief Create power-efficient constraints
     */
    static OptimizationConstraints createPowerEfficientConstraints(int device_id);
};

/**
 * @brief Utility functions for optimization
 */
namespace optimization_utils {

/**
 * @brief Convert string value to parameter value
 */
template<typename T>
T convertParameterValue(const std::string& value, const OptimizationParameter& param);

/**
 * @brief Calculate Pareto front from multiple objectives
 */
std::vector<std::map<std::string, std::string>> calculateParetoFront(
    const std::vector<std::tuple<std::map<std::string, std::string>, std::vector<double>>>& evaluations,
    const std::vector<bool>& maximize_objectives
);

/**
 * @brief Analyze optimization sensitivity
 */
struct SensitivityAnalysis {
    std::map<std::string, double> sensitivity_scores;
    std::map<std::string, double> parameter_importance;
    std::vector<std::string> most_sensitive_parameters;
    std::vector<std::string> least_sensitive_parameters;
};

SensitivityAnalysis analyzeSensitivity(
    const std::vector<std::tuple<std::map<std::string, std::string>, double>>& evaluations,
    const std::vector<OptimizationParameter>& parameters
);

/**
 * @brief Generate optimization recommendations
 */
struct OptimizationRecommendation {
    std::string parameter_name;
    std::string current_value;
    std::string recommended_value;
    double expected_improvement{0.0};
    std::string reasoning;
    double confidence{0.0};
};

std::vector<OptimizationRecommendation> generateRecommendations(
    const OptimizationResult& result,
    const std::vector<OptimizationParameter>& parameters,
    const puzzle71::monitoring::PerformanceMetrics& current_metrics
);

/**
 * @brief Compare optimization algorithms
 */
struct AlgorithmComparison {
    std::map<std::string, OptimizationResult> results;
    std::string best_algorithm;
    double best_objective_value{std::numeric_limits<double>::lowest()};
    std::chrono::milliseconds fastest_time{std::chrono::milliseconds::max()};
};

AlgorithmComparison compareAlgorithms(
    int device_id,
    const std::vector<OptimizationParameter>& parameters,
    const OptimizationConstraints& constraints,
    OptimizationObjective objective,
    const std::vector<std::string>& algorithm_names,
    std::function<std::map<std::string, std::string>(const std::map<std::string, std::string>&)> evaluation_function
);

} // namespace optimization_utils

} // namespace puzzle71::optimization