
node("windows-1") {
  stage("Build CASM for Windows") {
    git url: 'http://172.20.1.41/MUON-III/muon-casm.git'
    dir("build"){
      if(fileExists("Release")) {
        bat 'rmdir "Release" /S /Q'
      }
      bat 'cmake .. -G "Visual Studio 17 2022" -A x64'
      bat 'cmake --build . --config Release -- /nologo /verbosity:minimal /maxcpucount'
      archiveArtifacts artifacts: 'Release/casm.exe', fingerprint: true
      //deleteDir()
    }
    
    withCredentials([string(credentialsId: 'muon-discord-webhook', variable: 'DISCORD_URL')]) {
     discordSend description: "Build complete", footer: "windows", link: "$BUILD_URL", result: currentBuild.currentResult, title: JOB_NAME, webhookURL: DISCORD_URL
    }
  }
}
node("master") {
  stage("Build CASM-STATIC for Linux") {
    environment {
      CC  = '/usr/lib/ccache/gcc'
      CXX = '/usr/lib/ccache/g++'
    }
    git url: 'http://172.20.1.41/MUON-III/muon-casm.git'
    dir("build"){
      sh 'rm -f casm-static'
      sh 'cmake .. -DVERSION=latest -DBUILD_STATIC=true -DUSE_CCACHE=true -DMUST_USE_CCACHE=true'
      sh 'make -j4'
      archiveArtifacts artifacts: 'casm-static*', fingerprint: true
    }
    
    withCredentials([string(credentialsId: 'muon-discord-webhook', variable: 'DISCORD_URL')]) {
     discordSend description: "Build complete", footer: "linux", link: "$BUILD_URL", result: currentBuild.currentResult, title: JOB_NAME, webhookURL: DISCORD_URL
    }
  }
}
