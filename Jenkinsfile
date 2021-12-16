node("windows-1") {
  stage("Build CASM for Windows") {
    bat 'mkdir build'
    bat 'cd build && cmake .. -G "Unix Makefiles"'
    bat 'cd build && make'
  }
  
  post {
    always {
        archiveArtifacts artifacts: 'build/casm.exe', fingerprint: true
    }
    cleanup { 
        cleanWs() 
    }
  }
}
node("master") {
  stage("Build CASM-STATIC for Linux") {
    sh 'mkdir build'
    sh 'cd build && cmake .. -DUSE_GIT_VERSION=true -DBUILD_STATIC=true'
    sh 'cd build && make'
  }
  
  post {
    always {
        archiveArtifacts artifacts: 'build/casm-static*', fingerprint: true
    }
    cleanup { 
        cleanWs() 
    }
  }
}
