// Puzzle71 CUDA Technical Debt Repair System
// Jenkins Pipeline Configuration
// This file contains the global pipeline configuration

import jenkins.model.*
import hudson.model.*
import hudson.tasks.*
import jenkins.plugins.publishover.*

// Global pipeline configuration
def pipelineConfig = [
    projectName: 'Puzzle71 Technical Debt Repair',
    description: 'CUDA-based Bitcoin private key scanner with technical debt repairs',
    repository: 'https://github.com/your-org/puzzle71-keyhunt.git',
    defaultBranch: '002-techdebt-repair',

    // Build configuration
    buildConfig: [
        timeout: 120, // minutes
        retries: 3,
        parallelStages: [
            'Build Main',
            'Build Tests',
            'Build Benchmarks'
        ],
        testParallelStages: [
            'Unit Tests',
            'Integration Tests',
            'Performance Tests'
        ]
    ],

    // Performance thresholds
    performanceThresholds: [
        minThroughput: 1000.0, // keys/second
        maxMemoryUsage: 8.0,  // GB
        minGPUEfficiency: 90.0, // percent
        maxCompileTime: 30.0 // minutes
    ],

    // Quality gates
    qualityGates: [
        minTestCoverage: 85.0, // percent
        maxCodeDuplication: 5.0, // percent
        minConstitutionalCompliance: 95.0, // percent
        maxCriticalIssues: 0
    ],

    // Notification configuration
    notifications: [
        email: 'team@example.com',
        slackChannel: '#puzzle71-ci',
        onFailure: true,
        onUnstable: true,
        onSuccess: true
    ]
]

// Create folder structure if needed
def createFolderStructure() {
    def folderName = 'Puzzle71'

    if (!Jenkins.instance.getItem(folderName)) {
        println "Creating folder: ${folderName}"
        Jenkins.instance.createProject(FreeStyleProject, folderName)
    }
}

// Configure global tool installations
def configureTools() {
    // CMake configuration
    def cmakeInstallation = [
        name: 'CMake-3.28',
        home: '/usr/local/bin',
        properties: [
            hudson.tools.InstallSourceProperty.fromName('CMake-3.28.3')
        ]
    ]

    // CUDA configuration
    def cudaInstallation = [
        name: 'CUDA-11.8',
        home: '/usr/local/cuda',
        properties: [
            hudson.tools.InstallSourceProperty.fromName('CUDA-11.8')
        ]
    ]

    println "Tool configuration completed"
}

// Configure build agents
def configureAgents() {
    // GPU-enabled agents for CUDA builds
    def gpuAgentLabel = 'gpu-enabled'

    // Check if GPU agents are available
    def gpuAgents = Jenkins.instance.getComputer().getAll().findAll {
        computer -> computer.node.labelString.contains(gpuAgentLabel)
    }

    if (gpuAgents.isEmpty()) {
        println "WARNING: No GPU-enabled agents found. Pipeline may fail on CUDA compilation."
        println "Please configure agents with label '${gpuAgentLabel}' and NVIDIA GPU support."
    } else {
        println "Found ${gpuAgents.size()} GPU-enabled agents"
    }
}

// Configure credentials
def configureCredentials() {
    def credentialsId = 'puzzle71-github-credentials'

    // Check if credentials exist
    def credentials = com.cloudbees.plugins.credentials.CredentialsProvider.lookupCredentials(
        com.cloudbees.plugins.credentials.domains.Domain.global(),
        null,
        null,
        null
    ).findAll { it.id == credentialsId }

    if (credentials.isEmpty()) {
        println "WARNING: GitHub credentials not found. Please configure credentials with ID '${credentialsId}'"
    } else {
        println "GitHub credentials found: ${credentials.size()}"
    }
}

// Setup default views
def setupViews() {
    def listViewName = 'Puzzle71 Status'

    if (!Jenkins.instance.getView(listViewName)) {
        println "Creating view: ${listViewName}"
        def view = new hudson.model.ListView(listViewName)
        view.includeRegex('puzzle71.*')
        Jenkins.instance.addView(view)
    }
}

// Execute configuration
println "Configuring Puzzle71 CI/CD pipeline..."

createFolderStructure()
configureTools()
configureAgents()
configureCredentials()
setupViews()

println "Pipeline configuration completed successfully!"