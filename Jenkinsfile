pipeline {
  agent any
  stages {
    stage('Prepare') {
      steps {
        sh '''mkdir build
'''
        sh 'cd build && cmake ..'
      }
    }

    stage('Build') {
      steps {
        sh 'cd build && make'
      }
    }

  }
}